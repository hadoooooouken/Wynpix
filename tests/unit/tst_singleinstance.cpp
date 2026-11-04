#include <QSignalSpy>
#include <QTest>
#include <QThread>
#include <QUuid>

#include <atomic>
#include <memory>
#include <vector>

#include "components/singleinstance/singleinstancechannel.h"
#include "testsuites.h"

namespace {
using namespace Qt::StringLiterals;

// Short, so the "primary never answers" case does not slow the suite down.
constexpr SingleInstanceTimeouts kTestTimeouts{.forwardDeadlineMs = 500,
                                               .retryIntervalMs = 10};
// For the cases where the primary instance is late on purpose.
constexpr SingleInstanceTimeouts kPatientTimeouts{.forwardDeadlineMs = 5000,
                                                  .retryIntervalMs = 10};
// How long the primary instance in a test stays silent before it acts.
constexpr int kPrimaryDelayMs = 200;
// Upper bound of the waits for a secondary instance in a test.
constexpr int kSecondaryWaitMs = 10000;

// A server name no other test run or application instance uses.
QString uniqueServerName() {
  return SingleInstanceChannel::serverNameFor(
      QUuid::createUuid().toString(QUuid::WithoutBraces));
}

// A secondary instance on its own thread, as the mutex is owned per thread
// (two channels of one thread would share it) and the primary reads in this
// thread's event loop: on Windows the write completes only once the primary
// reads it. In the application the secondary is another process.
class SecondaryInstance {
public:
  SecondaryInstance(const QString &serverName, const QStringList &paths,
                    SingleInstanceTimeouts timeouts = kTestTimeouts)
      : thread(QThread::create([this, serverName, paths, timeouts]() {
          SingleInstanceChannel secondary(serverName, timeouts);
          claimed = secondary.claimRole();
          if (claimed != RoleClaim::Secondary)
            return;
          for (const QString &path : paths)
            outcomes.push_back(secondary.forwardToPrimary(path));
        })) {
    thread->start();
  }

  ~SecondaryInstance() { thread->wait(); }

  [[nodiscard]] bool isFinished() const { return thread->isFinished(); }

  // The role the secondary instance was given; valid once it has claimed it.
  [[nodiscard]] RoleClaim role() const { return claimed; }

  // Results of the forwards, in order; valid after finish().
  const std::vector<ForwardOutcome> &finish() {
    thread->wait();
    return outcomes;
  }

private:
  std::atomic<RoleClaim> claimed = RoleClaim::Failed;
  std::vector<ForwardOutcome> outcomes;
  std::unique_ptr<QThread> thread;
};

// A primary instance that already listens, with the paths it receives
// recorded.
struct ListeningPrimary {
  explicit ListeningPrimary(const QString &serverName)
      : channel(serverName, kTestTimeouts),
        received(&channel, &SingleInstanceChannel::pathReceived) {}

  SingleInstanceChannel channel;
  QSignalSpy received;
};
} // namespace

class SingleInstanceTests : public QObject {
  Q_OBJECT

private slots:
  void serverNameDependsOnTheTempPathOnly() {
    const QString first = SingleInstanceChannel::serverNameFor(u"C:/Temp"_s);
    QCOMPARE(SingleInstanceChannel::serverNameFor(u"C:/Temp"_s), first);
    QVERIFY(SingleInstanceChannel::serverNameFor(u"D:/Temp"_s) != first);
    QVERIFY(first.startsWith(u"wynpix-single-instance-"_s));
  }

  void theFirstProcessBecomesPrimary() {
    SingleInstanceChannel primary(uniqueServerName(), kTestTimeouts);
    QVERIFY(!primary.isPrimary());
    QVERIFY(primary.claimRole() == RoleClaim::Primary);
    QVERIFY(primary.isPrimary());
    // Claiming again keeps the decision.
    QVERIFY(primary.claimRole() == RoleClaim::Primary);
  }

  void aSecondProcessBecomesSecondaryEvenWithoutAListeningPrimary() {
    const QString serverName = uniqueServerName();
    SingleInstanceChannel owner(serverName, kTestTimeouts);
    QVERIFY(owner.claimRole() == RoleClaim::Primary);

    SecondaryInstance secondary(serverName, {});
    QTRY_VERIFY_WITH_TIMEOUT(secondary.isFinished(), kSecondaryWaitMs);
    QVERIFY(secondary.role() == RoleClaim::Secondary);
  }

  void listenRequiresThePrimaryRole() {
    SingleInstanceChannel channel(uniqueServerName(), kTestTimeouts);
    QVERIFY(!channel.listen());
  }

  void forwardingRequiresTheSecondaryRole() {
    SingleInstanceChannel primary(uniqueServerName(), kTestTimeouts);
    QVERIFY(primary.claimRole() == RoleClaim::Primary);
    QVERIFY(primary.forwardToPrimary(u"C:/a.png"_s) == ForwardOutcome::Failed);
  }

  void thePrimaryReceivesTheForwardedPath() {
    const QString serverName = uniqueServerName();
    ListeningPrimary primary(serverName);
    QVERIFY(primary.channel.claimRole() == RoleClaim::Primary);
    QVERIFY(primary.channel.listen());
    primary.channel.drainPendingPaths();

    SecondaryInstance secondary(serverName, {u"C:/images/a.png"_s});
    QVERIFY(primary.received.wait());
    QTRY_VERIFY_WITH_TIMEOUT(secondary.isFinished(), kSecondaryWaitMs);
    QVERIFY(secondary.finish() == std::vector{ForwardOutcome::Delivered});
    QCOMPARE(primary.received.count(), 1);
    QCOMPARE(primary.received.first().first().toString(), u"C:/images/a.png"_s);
  }

  void anEmptyPathAsksToRaise() {
    const QString serverName = uniqueServerName();
    ListeningPrimary primary(serverName);
    QVERIFY(primary.channel.claimRole() == RoleClaim::Primary);
    QVERIFY(primary.channel.listen());
    primary.channel.drainPendingPaths();

    SecondaryInstance secondary(serverName, {QString()});
    QVERIFY(primary.received.wait());
    QTRY_VERIFY_WITH_TIMEOUT(secondary.isFinished(), kSecondaryWaitMs);
    QVERIFY(secondary.finish() == std::vector{ForwardOutcome::Delivered});
    QVERIFY(primary.received.first().first().toString().isEmpty());
  }

  void everyForwardIsDelivered() {
    const QString serverName = uniqueServerName();
    ListeningPrimary primary(serverName);
    QVERIFY(primary.channel.claimRole() == RoleClaim::Primary);
    QVERIFY(primary.channel.listen());
    primary.channel.drainPendingPaths();

    SecondaryInstance secondary(serverName, {u"C:/a.png"_s, u"C:/b.png"_s});
    QTRY_COMPARE(primary.received.count(), 2);
    QTRY_VERIFY_WITH_TIMEOUT(secondary.isFinished(), kSecondaryWaitMs);
    QVERIFY(secondary.finish() ==
            (std::vector{ForwardOutcome::Delivered, ForwardOutcome::Delivered}));
    QStringList received{primary.received.at(0).first().toString(),
                         primary.received.at(1).first().toString()};
    received.sort();
    QCOMPARE(received, (QStringList{u"C:/a.png"_s, u"C:/b.png"_s}));
  }

  void pathsWaitUntilTheyAreDrained() {
    const QString serverName = uniqueServerName();
    ListeningPrimary primary(serverName);
    QVERIFY(primary.channel.claimRole() == RoleClaim::Primary);
    QVERIFY(primary.channel.listen());

    SecondaryInstance early(serverName, {u"C:/early1.png"_s, u"C:/early2.png"_s});
    QTRY_VERIFY_WITH_TIMEOUT(early.isFinished(), kSecondaryWaitMs);
    QVERIFY(early.finish() ==
            (std::vector{ForwardOutcome::Delivered, ForwardOutcome::Delivered}));
    // Acknowledged, but nobody is ready to act on them yet.
    QCOMPARE(primary.received.count(), 0);

    primary.channel.drainPendingPaths();
    QCOMPARE(primary.received.count(), 2);
    QCOMPARE(primary.received.at(0).first().toString(), u"C:/early1.png"_s);
    QCOMPARE(primary.received.at(1).first().toString(), u"C:/early2.png"_s);

    // From now on paths are passed on at once.
    SecondaryInstance late(serverName, {u"C:/late.png"_s});
    QTRY_COMPARE(primary.received.count(), 3);
    QCOMPARE(primary.received.at(2).first().toString(), u"C:/late.png"_s);
  }

  void aSecondaryWaitsForAPrimaryThatStartsListeningLate() {
    const QString serverName = uniqueServerName();
    ListeningPrimary primary(serverName);
    QVERIFY(primary.channel.claimRole() == RoleClaim::Primary);
    primary.channel.drainPendingPaths();

    // The primary owns the role but its pipe does not exist yet: the launch
    // during the start-up of the real application.
    SecondaryInstance secondary(serverName, {u"C:/images/a.png"_s}, kPatientTimeouts);
    QTest::qWait(kPrimaryDelayMs);
    QVERIFY(!secondary.isFinished());
    QVERIFY(primary.channel.listen());

    QTRY_VERIFY_WITH_TIMEOUT(secondary.isFinished(), kSecondaryWaitMs);
    QVERIFY(secondary.role() == RoleClaim::Secondary);
    QVERIFY(secondary.finish() == std::vector{ForwardOutcome::Delivered});
    QCOMPARE(primary.received.count(), 1);
  }

  void aSecondaryGivesUpWithoutStartingAnotherInstance() {
    const QString serverName = uniqueServerName();
    SingleInstanceChannel primary(serverName, kTestTimeouts);
    QVERIFY(primary.claimRole() == RoleClaim::Primary);

    // The primary never listens; the secondary reports failure instead of
    // becoming a second primary.
    SecondaryInstance secondary(serverName, {u"C:/images/a.png"_s});
    QTRY_VERIFY_WITH_TIMEOUT(secondary.isFinished(), kSecondaryWaitMs);
    QVERIFY(secondary.role() == RoleClaim::Secondary);
    QVERIFY(secondary.finish() == std::vector{ForwardOutcome::Failed});
  }

  void aSecondaryTakesOverWhenThePrimaryDisappears() {
    const QString serverName = uniqueServerName();
    auto primary = std::make_unique<SingleInstanceChannel>(serverName, kTestTimeouts);
    QVERIFY(primary->claimRole() == RoleClaim::Primary);

    SecondaryInstance secondary(serverName, {u"C:/images/a.png"_s}, kPatientTimeouts);
    QTRY_VERIFY_WITH_TIMEOUT(secondary.role() == RoleClaim::Secondary,
                             kSecondaryWaitMs);
    QTest::qWait(kPrimaryDelayMs);
    QVERIFY(!secondary.isFinished());

    primary.reset();
    QTRY_VERIFY_WITH_TIMEOUT(secondary.isFinished(), kSecondaryWaitMs);
    QVERIFY(secondary.finish() == std::vector{ForwardOutcome::BecamePrimary});
  }
};

int runSingleInstanceTests(int argc, char **argv) {
  SingleInstanceTests tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "tst_singleinstance.moc"
