#include <QBuffer>
#include <QImage>
#include <QImageReader>
#include <QTest>
#include <QThread>

#include <atomic>
#include <memory>
#include <optional>
#include <stop_token>

#include "testsuites.h"
#include "utils/decodebudget.h"

namespace {
constexpr qint64 kMiB = 1024 * 1024;
// How long a reservation that must wait is given to (wrongly) get through.
constexpr int kBlockedWindowMs = 150;
constexpr int kReleaseTimeoutMs = 5000;
constexpr QSize kPngSize(10, 20);
constexpr qint64 kArgb32BytesPerPixel = 4;
} // namespace

class DecodeBudgetTests : public QObject {
  Q_OBJECT

private slots:
  void aReservationThatFitsIsGrantedAtOnce() {
    DecodeBudget budget(2 * kMiB);
    const auto first = budget.reserve(kMiB, {});
    const auto second = budget.reserve(kMiB, {});
    QVERIFY(first);
    QVERIFY(second);
  }

  void aReservationWaitsUntilEnoughIsReturned() {
    DecodeBudget budget(2 * kMiB);
    auto held = budget.reserve(2 * kMiB, {});
    QVERIFY(held);
    std::atomic<bool> granted = false;
    std::unique_ptr<QThread> waiter(QThread::create([&] {
      const auto reservation = budget.reserve(kMiB, {});
      granted = reservation.has_value();
    }));
    waiter->start();
    QTest::qWait(kBlockedWindowMs);
    QVERIFY(!granted);

    held.reset();
    QVERIFY(waiter->wait(kReleaseTimeoutMs));
    QVERIFY(granted);
  }

  // An image larger than the budget still gets its thumbnail, alone.
  void aReservationLargerThanTheBudgetIsGrantedWhenItIsFree() {
    DecodeBudget budget(kMiB);
    QVERIFY(budget.reserve(10 * kMiB, {}));
  }

  void aZeroByteReservationDoesNotWait() {
    DecodeBudget budget(kMiB);
    const auto held = budget.reserve(kMiB, {});
    QVERIFY(held);
    QVERIFY(budget.reserve(0, {}));
  }

  void aCancelledWaitReturnsNothing() {
    DecodeBudget budget(kMiB);
    const auto held = budget.reserve(kMiB, {});
    std::stop_source stop;
    stop.request_stop();
    QVERIFY(!budget.reserve(kMiB, DecodeContext{stop.get_token()}));
  }

  void aMovedReservationReturnsItsBytesOnce() {
    DecodeBudget budget(kMiB);
    {
      std::optional<DecodeBudget::Reservation> target = budget.reserve(kMiB, {});
      target = budget.reserve(0, {});
    }
    std::stop_source stop;
    stop.request_stop();
    const auto again = budget.reserve(kMiB, DecodeContext{stop.get_token()});
    QVERIFY(again);
    QVERIFY(!budget.reserve(kMiB, DecodeContext{stop.get_token()}));
  }

  void theEstimateComesFromTheHeader() {
    QImage image(kPngSize, QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    QVERIFY(image.save(&buffer, "png"));
    buffer.close();
    buffer.open(QIODevice::ReadOnly);

    QImageReader reader(&buffer, "png");
    QCOMPARE(estimateDecodedBytes(reader),
             std::optional<qint64>(kPngSize.width() * kPngSize.height() *
                                   kArgb32BytesPerPixel));
  }

  void noEstimateWithoutDimensions() {
    QBuffer buffer;
    buffer.open(QIODevice::ReadOnly);
    QImageReader reader(&buffer, "png");
    QCOMPARE(estimateDecodedBytes(reader), std::optional<qint64>());
  }
};

int runDecodeBudgetTests(int argc, char **argv) {
  DecodeBudgetTests tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "tst_decodebudget.moc"
