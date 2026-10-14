#include <QCoreApplication>
#include <QTest>
#include <QThread>

#include <memory>
#include <vector>

#include "gui/quick/render/rendererrorchannel.h"
#include "testsuites.h"

// RenderErrorChannel hands render-thread errors to the GUI thread. The render
// tests render on the test thread (QQuickRenderControl), so here a real
// worker thread posts and the sink records where it runs.

namespace {
using namespace Qt::StringLiterals;

// Threads posting at the same time.
constexpr int kPostingThreads = 4;

struct Delivery {
  QString message;
  QThread *thread = nullptr;
};
} // namespace

class RenderErrorChannelTests : public QObject {
  Q_OBJECT

private slots:
  void messagesPostedOnAWorkerReachTheSinkOnTheGuiThread() {
    QObject receiver;
    QList<Delivery> deliveries;
    const auto channel = std::make_shared<RenderErrorChannel>(
        &receiver, [&deliveries](const QString &message) {
          deliveries.append({message, QThread::currentThread()});
        });

    std::unique_ptr<QThread> worker(
        QThread::create([channel]() { channel->post(u"device lost"_s); }));
    worker->start();
    QVERIFY(worker->wait());
    // Nothing is delivered on the posting thread.
    QVERIFY(deliveries.isEmpty());

    QCoreApplication::processEvents();
    QCOMPARE(deliveries.size(), 1);
    QCOMPARE(deliveries.first().message, u"device lost"_s);
    QCOMPARE(deliveries.first().thread, QCoreApplication::instance()->thread());
  }

  void everyMessageOfConcurrentPostersIsDeliveredOnce() {
    QObject receiver;
    QStringList delivered;
    QThread *gui = QCoreApplication::instance()->thread();
    bool offGuiThread = false;
    const auto channel = std::make_shared<RenderErrorChannel>(
        &receiver, [&](const QString &message) {
          offGuiThread = offGuiThread || QThread::currentThread() != gui;
          delivered.append(message);
        });

    std::vector<std::unique_ptr<QThread>> workers;
    for (int i = 0; i < kPostingThreads; ++i) {
      workers.emplace_back(QThread::create(
          [channel, i]() { channel->post(u"error %1"_s.arg(i)); }));
      workers.back()->start();
    }
    for (const auto &worker : workers)
      QVERIFY(worker->wait());
    QVERIFY(delivered.isEmpty());

    QCoreApplication::processEvents();
    QVERIFY(!offGuiThread);
    delivered.sort();
    QStringList expected;
    for (int i = 0; i < kPostingThreads; ++i)
      expected.append(u"error %1"_s.arg(i));
    QCOMPARE(delivered, expected);
  }

  void aDestroyedReceiverDropsPendingMessages() {
    auto receiver = std::make_unique<QObject>();
    int deliveries = 0;
    const auto channel = std::make_shared<RenderErrorChannel>(
        receiver.get(), [&deliveries](const QString &) { ++deliveries; });

    std::unique_ptr<QThread> worker(
        QThread::create([channel]() { channel->post(u"late"_s); }));
    worker->start();
    QVERIFY(worker->wait());
    receiver.reset();

    QCoreApplication::processEvents();
    QCOMPARE(deliveries, 0);
  }
};

int runRenderErrorChannelTests(int argc, char **argv) {
  RenderErrorChannelTests tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "tst_rendererrorchannel.moc"
