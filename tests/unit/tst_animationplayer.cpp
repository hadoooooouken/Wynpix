#include <QDir>
#include <QFile>
#include <QMovie>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <cmath>
#include <vector>

#include "components/animationplayer/animationplayer.h"
#include "testanimations.h"
#include "testsuites.h"

namespace {
using namespace Qt::StringLiterals;
using namespace TestAnimations;

const QByteArray kGifFormat = "gif"_ba;
// Generous upper bound for one frame delay on a busy machine.
constexpr int kFrameTimeoutMs = 2000;
// Longer than every frame delay: nothing may happen within it.
constexpr int kQuietPeriodMs = 200;
// Leading bytes of kThreeFrameGif in a truncated copy: the first two frames
// are whole and the third one's image descriptor is, but none of its image
// data (QMovie still counts three frames and decodes two).
constexpr qsizetype kTruncatedThreeFrameSize = 140;
// The frames QMovie decodes from that copy.
constexpr int kTruncatedDecodableFrames = 2;

QString writeFile(const QTemporaryDir &dir, const QString &name,
                  const unsigned char *data, qsizetype size) {
  const QString path = dir.filePath(name);
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly) ||
      file.write(reinterpret_cast<const char *>(data), size) != size)
    return {};
  return path;
}

// The delay ImageViewerV2 schedules after each frame: QMovie's own.
std::vector<int> qmovieDelays(const QString &path) {
  QMovie movie(path, kGifFormat);
  std::vector<int> delays;
  movie.jumpToFrame(0);
  for (int frame = 0; frame < movie.frameCount(); ++frame) {
    delays.push_back(movie.nextFrameDelay());
    movie.jumpToNextFrame();
  }
  return delays;
}
} // namespace

class AnimationPlayerTests : public QObject {
  Q_OBJECT

private slots:
  void initTestCase() {
    QVERIFY(mDir.isValid());
    mThreeFrames = writeFile(mDir, u"three.gif"_s, kThreeFrameGif,
                             sizeof(kThreeFrameGif));
    mSingleFrame = writeFile(mDir, u"single.gif"_s, kSingleFrameGif,
                             sizeof(kSingleFrameGif));
    QVERIFY(!mThreeFrames.isEmpty());
    QVERIFY(!mSingleFrame.isEmpty());
  }

  void openPublishesFirstFrameAndPlays() {
    AnimationPlayer player;
    QSignalSpy frames(&player, &AnimationPlayer::frameReady);
    QVERIFY(player.open(mThreeFrames, kGifFormat));
    QCOMPARE(frames.count(), 1);
    QCOMPARE(frames.first().at(1).toInt(), 0);
    QCOMPARE(player.frameCount(), kThreeFrameCount);
    QCOMPARE(player.frameIndex(), 0);
    QCOMPARE(player.frameSize(), QSize(4, 4));
    QVERIFY(player.isPlaying());
    QCOMPARE(player.currentFrame()->pixelColor(0, 0), QColor(Qt::red));
  }

  // Every frame stays for QMovie::nextFrameDelay(), as in the widget viewer.
  void scheduleMatchesMovieDelays() {
    const std::vector<int> expected = qmovieDelays(mThreeFrames);
    QCOMPARE(expected, kThreeFrameDelays);
    AnimationPlayer player;
    player.setLoop(true);
    std::vector<int> delays;
    std::vector<int> order;
    connect(&player, &AnimationPlayer::frameReady, this,
            [&](const std::shared_ptr<const QImage> &, int index) {
              order.push_back(index);
              delays.push_back(player.scheduledDelay());
            });
    QVERIFY(player.open(mThreeFrames, kGifFormat));
    // Frames 0, 1, 2 and the loop back to 0.
    QTRY_VERIFY_WITH_TIMEOUT(order.size() >= expected.size() + 1,
                             kFrameTimeoutMs * kThreeFrameCount);
    for (std::size_t i = 0; i <= expected.size(); ++i) {
      QCOMPARE(order[i], int(i % expected.size()));
      QCOMPARE(delays[i], expected[i % expected.size()]);
    }
  }

  void withoutLoopStopsAtLastFrame() {
    AnimationPlayer player;
    QSignalSpy finished(&player, &AnimationPlayer::playbackFinished);
    QVERIFY(player.open(mThreeFrames, kGifFormat));
    QVERIFY(finished.wait(kFrameTimeoutMs * kThreeFrameCount));
    QVERIFY(!player.isPlaying());
    QVERIFY(player.isFinished());
    QCOMPARE(player.frameIndex(), kLastFrame);
    QCOMPARE(player.scheduledDelay(), AnimationPlayer::kNoDelay);

    // Turning the loop on resumes playback, like ImageViewerV2.
    QSignalSpy frames(&player, &AnimationPlayer::frameReady);
    player.setLoop(true);
    QVERIFY(player.isPlaying());
    QVERIFY(!player.isFinished());
    QVERIFY(frames.wait(kFrameTimeoutMs));
    QCOMPARE(frames.first().at(1).toInt(), 0);
  }

  void steppingWrapsAndKeepsPause() {
    AnimationPlayer player;
    QVERIFY(player.open(mThreeFrames, kGifFormat));
    player.pause();
    QVERIFY(!player.isPlaying());
    player.prevFrame();
    QCOMPARE(player.frameIndex(), kLastFrame);
    QCOMPARE(player.currentFrame()->pixelColor(0, 0), QColor(Qt::blue));
    player.nextFrame();
    QCOMPARE(player.frameIndex(), 0);
    player.nextFrame();
    QCOMPARE(player.frameIndex(), 1);
    QVERIFY(!player.isPlaying());
    QVERIFY(player.showFrame(0));
    QCOMPARE(player.currentFrame()->pixelColor(0, 0), QColor(Qt::red));
    QVERIFY(!player.showFrame(kThreeFrameCount));
    QSignalSpy frames(&player, &AnimationPlayer::frameReady);
    QTest::qWait(kQuietPeriodMs);
    QCOMPARE(frames.count(), 0);
  }

  void singleFrameDoesNotPlay() {
    AnimationPlayer player;
    QVERIFY(player.open(mSingleFrame, kGifFormat));
    QCOMPARE(player.frameIndex(), 0);
    QVERIFY(!player.isPlaying());
    player.play();
    QVERIFY(!player.isPlaying());
  }

  void unreadableFileReportsError() {
    AnimationPlayer player;
    QSignalSpy errors(&player, &AnimationPlayer::playbackError);
    QVERIFY(!player.open(mDir.filePath(u"missing.gif"_s), kGifFormat));
    QCOMPARE(errors.count(), 1);
    const auto error = errors.first().first().value<AnimationPlaybackError>();
    QCOMPARE(error.kind, AnimationPlaybackError::Kind::Open);
    QCOMPARE(error.filePath, mDir.filePath(u"missing.gif"_s));
    QVERIFY(!player.isOpen());
    QCOMPARE(player.frameIndex(), AnimationPlayer::kNoFrame);
  }

  // A-15: QMovie decodes a truncated GIF without an error but restarts at
  // the first frame before the declared last one. Without looping, playback
  // must still end (the slideshow waits for it) after the decodable frames.
  void truncatedFileEndsAfterItsDecodableFrames() {
    const QString path = writeTruncatedThreeFrameGif();
    QVERIFY(!path.isEmpty());
    AnimationPlayer player;
    QSignalSpy errors(&player, &AnimationPlayer::playbackError);
    QSignalSpy finished(&player, &AnimationPlayer::playbackFinished);
    QVERIFY(player.open(path, kGifFormat));
    QCOMPARE(player.frameCount(), kThreeFrameCount);
    QVERIFY(player.isPlaying());
    QVERIFY(finished.wait(kFrameTimeoutMs * kThreeFrameCount));
    QCOMPARE(finished.count(), 1);
    QVERIFY(player.isFinished());
    QVERIFY(!player.isPlaying());
    QCOMPARE(errors.count(), 0);
    // Reaching the end of the data leaves the decodable frames as the whole
    // animation, stopped on its last frame.
    QCOMPARE(player.frameCount(), kTruncatedDecodableFrames);
    QCOMPARE(player.frameIndex(), kTruncatedDecodableFrames - 1);
  }

  // Looping, the truncated file keeps playing its decodable frames.
  void truncatedFileLoopsItsDecodableFrames() {
    const QString path = writeTruncatedThreeFrameGif();
    QVERIFY(!path.isEmpty());
    AnimationPlayer player;
    player.setLoop(true);
    QSignalSpy frames(&player, &AnimationPlayer::frameReady);
    QSignalSpy finished(&player, &AnimationPlayer::playbackFinished);
    QVERIFY(player.open(path, kGifFormat));
    // The first frame, the second, and the first again after the wrap.
    QTRY_VERIFY_WITH_TIMEOUT(frames.count() >= kThreeFrameCount,
                             kFrameTimeoutMs * kThreeFrameCount);
    QCOMPARE(frames.at(kThreeFrameCount - 1).at(1).toInt(), 0);
    QVERIFY(player.isPlaying());
    QCOMPARE(finished.count(), 0);
  }

  // Stepping to a frame past the decodable part used to step forever (the
  // GUI thread hung); it now shows the last decodable frame.
  void steppingPastTruncatedEndShowsLastDecodableFrame() {
    const QString path = writeTruncatedThreeFrameGif();
    QVERIFY(!path.isEmpty());
    AnimationPlayer player;
    QVERIFY(player.open(path, kGifFormat));
    player.pause();
    QSignalSpy errors(&player, &AnimationPlayer::playbackError);
    QSignalSpy counts(&player, &AnimationPlayer::frameCountChanged);
    QVERIFY(player.showFrame(kLastFrame));
    QCOMPARE(errors.count(), 0);
    QCOMPARE(counts.count(), 1);
    QCOMPARE(player.frameCount(), kTruncatedDecodableFrames);
    QCOMPARE(player.frameIndex(), kTruncatedDecodableFrames - 1);
    // Stepping wraps around the decodable frames.
    player.nextFrame();
    QCOMPARE(player.frameIndex(), 0);
    player.prevFrame();
    QCOMPARE(player.frameIndex(), kTruncatedDecodableFrames - 1);
    QCOMPARE(errors.count(), 0);
  }

  // Reversed, the truncated file loops its decodable frames backwards.
  void truncatedFilePlaysBackwards() {
    const QString path = writeTruncatedThreeFrameGif();
    QVERIFY(!path.isEmpty());
    AnimationPlayer player;
    player.setLoop(true);
    QSignalSpy errors(&player, &AnimationPlayer::playbackError);
    std::vector<int> order;
    connect(&player, &AnimationPlayer::frameReady, this,
            [&](const std::shared_ptr<const QImage> &, int index) {
              order.push_back(index);
            });
    QVERIFY(player.open(path, kGifFormat));
    player.setReversed(true);
    QTRY_VERIFY_WITH_TIMEOUT(order.size() >= 4,
                             kFrameTimeoutMs * kThreeFrameCount);
    QCOMPARE(order[0], 0);
    QCOMPARE(order[1], 1);
    QCOMPARE(order[2], 0);
    QCOMPARE(order[3], 1);
    QVERIFY(player.isPlaying());
    QCOMPARE(errors.count(), 0);
  }

  void closeStopsPlayback() {
    AnimationPlayer player;
    QVERIFY(player.open(mThreeFrames, kGifFormat));
    player.close();
    QVERIFY(!player.isPlaying());
    QVERIFY(!player.currentFrame());
    QCOMPARE(player.frameCount(), 0);
    QSignalSpy frames(&player, &AnimationPlayer::frameReady);
    QTest::qWait(kQuietPeriodMs);
    QCOMPARE(frames.count(), 0);
  }

  void speedScalingModifiesScheduledDelay() {
    AnimationPlayer player;
    QSignalSpy speedSpy(&player, &AnimationPlayer::speedChanged);
    QVERIFY(player.open(mThreeFrames, kGifFormat));
    player.pause();
    QCOMPARE(player.speed(), 1.0);

    // Speed up 2x: delay should be halved.
    player.setSpeed(2.0);
    QCOMPARE(speedSpy.count(), 1);
    QCOMPARE(player.speed(), 2.0);
    player.play();
    const int baseDelay0 = kThreeFrameDelays[0];
    const int expected2x = std::max(1, static_cast<int>(std::round(baseDelay0 / 2.0)));
    QCOMPARE(player.scheduledDelay(), expected2x);

    // Slow down 0.5x: delay should be doubled.
    player.setSpeed(0.5);
    QCOMPARE(speedSpy.count(), 2);
    QCOMPARE(player.speed(), 0.5);
    const int expectedHalfX = std::max(1, static_cast<int>(std::round(baseDelay0 / 0.5)));
    QCOMPARE(player.scheduledDelay(), expectedHalfX);
  }

  void reversedPlaybackPlaysBackwards() {
    AnimationPlayer player;
    QSignalSpy reversedSpy(&player, &AnimationPlayer::reversedChanged);
    player.setLoop(true);

    std::vector<int> order;
    connect(&player, &AnimationPlayer::frameReady, this,
            [&](const std::shared_ptr<const QImage> &, int index) {
              order.push_back(index);
            });

    QVERIFY(player.open(mThreeFrames, kGifFormat));
    // Opening resets the direction unless the settings are remembered, so
    // it is set on the open file.
    player.setReversed(true);
    QCOMPARE(reversedSpy.count(), 1);
    QVERIFY(player.isReversed());
    // Start at frame 0, next step reversed loops to 2, then 1, 0, 2...
    QTRY_VERIFY_WITH_TIMEOUT(order.size() >= 4,
                             kFrameTimeoutMs * kThreeFrameCount);
    QCOMPARE(order[0], 0);
    QCOMPARE(order[1], 2);
    QCOMPARE(order[2], 1);
    QCOMPARE(order[3], 0);

    // Test firstFrame() and lastFrame() helpers
    player.pause();
    player.firstFrame();
    QCOMPARE(player.frameIndex(), 0);
    player.lastFrame();
    QCOMPARE(player.frameIndex(), kLastFrame);
  }

  void rememberSettingsPreservesStateAcrossOpenClose() {
    AnimationPlayer player;
    QSignalSpy rememberSpy(&player, &AnimationPlayer::rememberSettingsChanged);

    // Default: rememberSettings is false -> close() resets speed and reversed.
    QVERIFY(!player.rememberSettings());
    QVERIFY(player.open(mThreeFrames, kGifFormat));
    player.setSpeed(2.0);
    player.setReversed(true);
    player.close();
    QCOMPARE(player.speed(), 1.0);
    QVERIFY(!player.isReversed());

    // With rememberSettings true: close() preserves speed and reversed.
    player.setRememberSettings(true);
    QCOMPARE(rememberSpy.count(), 1);
    QVERIFY(player.rememberSettings());
    QVERIFY(player.open(mThreeFrames, kGifFormat));
    player.setSpeed(3.0);
    player.setReversed(true);
    player.close();
    QCOMPARE(player.speed(), 3.0);
    QVERIFY(player.isReversed());

    // Opening another animation preserves settings.
    QVERIFY(player.open(mThreeFrames, kGifFormat));
    QCOMPARE(player.speed(), 3.0);
    QVERIFY(player.isReversed());
  }

  void restartAfterFinishedInBothDirections() {
    AnimationPlayer player;
    // 1. Forward finish -> restart forward
    QSignalSpy finishedForward(&player, &AnimationPlayer::playbackFinished);
    QVERIFY(player.open(mThreeFrames, kGifFormat));
    QVERIFY(finishedForward.wait(kFrameTimeoutMs * kThreeFrameCount));
    QVERIFY(player.isFinished());
    QCOMPARE(player.frameIndex(), kLastFrame);
    QCOMPARE(player.scheduledDelay(), AnimationPlayer::kNoDelay);

    // play() restarts from frame 0
    player.play();
    QVERIFY(player.isPlaying());
    QVERIFY(!player.isFinished());
    QCOMPARE(player.frameIndex(), 0);
    QVERIFY(player.scheduledDelay() > 0);

    // 2. Forward finish -> switch to reverse -> restart without early-exit stall
    player.close();
    QVERIFY(player.open(mThreeFrames, kGifFormat));
    QSignalSpy finished2(&player, &AnimationPlayer::playbackFinished);
    QVERIFY(finished2.wait(kFrameTimeoutMs * kThreeFrameCount));
    QVERIFY(player.isFinished());
    QCOMPARE(player.frameIndex(), kLastFrame);

    player.setReversed(true);
    // Already at startFrame (kLastFrame): play() must start timer without getting stuck.
    player.play();
    QVERIFY(player.isPlaying());
    QVERIFY(!player.isFinished());
    QCOMPARE(player.frameIndex(), kLastFrame);
    QVERIFY(player.scheduledDelay() > 0);

    // 3. Reverse finish -> switch to forward -> restart without early-exit stall
    QSignalSpy finishedReverse(&player, &AnimationPlayer::playbackFinished);
    QVERIFY(finishedReverse.wait(kFrameTimeoutMs * kThreeFrameCount));
    QVERIFY(player.isFinished());
    QCOMPARE(player.frameIndex(), 0);

    player.setReversed(false);
    // Already at startFrame (0): play() must start timer without getting stuck.
    player.play();
    QVERIFY(player.isPlaying());
    QVERIFY(!player.isFinished());
    QCOMPARE(player.frameIndex(), 0);
    QVERIFY(player.scheduledDelay() > 0);
  }

private:
  QString writeTruncatedThreeFrameGif() const {
    return writeFile(mDir, u"truncated.gif"_s, kThreeFrameGif,
                     kTruncatedThreeFrameSize);
  }

  QTemporaryDir mDir;
  QString mThreeFrames;
  QString mSingleFrame;
};

int runAnimationPlayerTests(int argc, char **argv) {
  AnimationPlayerTests tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "tst_animationplayer.moc"
