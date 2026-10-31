#include "animationplayer.h"

#include <QImageReader>
#include <QMovie>
#include <algorithm>
#include <cmath>

namespace {
// QMovie numbers its frames from 0; animations with fewer frames than this
// are shown, not played.
constexpr int kFirstFrame = 0;
constexpr int kMinimumPlayableFrames = 2;
// Animations whose uncompressed frames fit within this threshold use CacheAll
// for instant random access, scrub and reverse. Larger files stay on CacheNone
// to protect physical RAM.
constexpr qint64 kMaxCacheBytes = 256 * 1024 * 1024;
} // namespace

//------------------------------------------------------------------------------
AnimationPlayer::AnimationPlayer(QObject *parent) : QObject(parent) {
  mTimer.setSingleShot(true);
  // Frame delays are short; the default coarse timer would drift by up to 5%.
  mTimer.setTimerType(Qt::PreciseTimer);
  connect(&mTimer, &QTimer::timeout, this, &AnimationPlayer::onTimer);
}

AnimationPlayer::~AnimationPlayer() = default;

//------------------------------------------------------------------------------
bool AnimationPlayer::open(const QString &path, const QByteArray &format) {
  close();
  auto movie = std::make_unique<QMovie>(path, format);
  if (!movie->isValid()) {
    emit playbackError({.kind = AnimationPlaybackError::Kind::Open,
                        .filePath = path,
                        .reason = movie->lastErrorString()});
    return false;
  }

  // QMovie requires setCacheMode() to be configured before reading frames.
  // Setting it after jumpToFrame() leaves frame 0 un-cached, breaking loop jumps.
  const QSize size = QImageReader(path, format).size();
  const qint64 estimatedBytes = size.isValid()
      ? static_cast<qint64>(size.width()) * size.height() * 4 * movie->frameCount()
      : 0;
  if (estimatedBytes > 0 && estimatedBytes <= kMaxCacheBytes)
    movie->setCacheMode(QMovie::CacheAll);

  // Frames are advanced by the player's own timer; the QMovie is never
  // started.
  mMovie = std::move(movie);
  mFrameCount = mMovie->frameCount();
  if (!mMovie->jumpToFrame(kFirstFrame) || mMovie->currentImage().isNull()) {
    reportDecodeError(kFirstFrame);
    close();
    return false;
  }

  emit frameCountChanged();
  // Playing before the first frame is published, so that its delay is
  // already scheduled when frameReady() arrives.
  if (mFrameCount >= kMinimumPlayableFrames)
    setPlaying(true);
  publishCurrentFrame();
  return true;
}

void AnimationPlayer::close() {
  mTimer.stop();
  const bool wasOpen = mMovie != nullptr;
  mMovie.reset();
  mFrame.reset();
  mFrameCount = 0;
  setPlaying(false);
  setFinished(false);
  if (!mRememberSettings) {
    if (mReversed) {
      mReversed = false;
      emit reversedChanged();
    }
    if (!qFuzzyCompare(mSpeed, 1.0)) {
      mSpeed = 1.0;
      emit speedChanged();
    }
  }
  if (wasOpen) {
    emit frameCountChanged();
    emit frameChanged();
  }
}

bool AnimationPlayer::isOpen() const { return mMovie != nullptr; }

//------------------------------------------------------------------------------
void AnimationPlayer::play() {
  if (!mMovie || mFrameCount < kMinimumPlayableFrames)
    return;
  const int startFrame = mReversed ? mFrameCount - 1 : kFirstFrame;
  const int endFrame = mReversed ? kFirstFrame : mFrameCount - 1;
  const bool restartNeeded =
      mFinished || (!mLoop && mMovie->currentFrameNumber() == endFrame);
  setFinished(false);
  setPlaying(true);
  if (restartNeeded && mMovie->currentFrameNumber() != startFrame) {
    if (!showFrame(startFrame))
      pause();
  } else {
    scheduleNextFrame();
  }
}

void AnimationPlayer::pause() {
  mTimer.stop();
  setPlaying(false);
}

void AnimationPlayer::togglePlayPause() {
  if (isPlaying())
    pause();
  else
    play();
}

bool AnimationPlayer::isPlaying() const { return mPlaying; }

void AnimationPlayer::setLoop(bool loop) {
  if (mLoop == loop)
    return;
  mLoop = loop;
  emit loopChanged();
  // ImageViewerV2::setLoopPlayback(): enabling the loop restarts playback.
  if (mLoop)
    play();
}

bool AnimationPlayer::loops() const { return mLoop; }

void AnimationPlayer::setSpeed(qreal speed) {
  if (speed <= 0.0 || qFuzzyCompare(mSpeed, speed))
    return;
  mSpeed = speed;
  emit speedChanged();
  if (mPlaying)
    scheduleNextFrame();
}

qreal AnimationPlayer::speed() const { return mSpeed; }

void AnimationPlayer::setReversed(bool reversed) {
  if (mReversed == reversed)
    return;
  mReversed = reversed;
  emit reversedChanged();
}

bool AnimationPlayer::isReversed() const { return mReversed; }

void AnimationPlayer::setRememberSettings(bool remember) {
  if (mRememberSettings == remember)
    return;
  mRememberSettings = remember;
  emit rememberSettingsChanged();
}

bool AnimationPlayer::rememberSettings() const { return mRememberSettings; }

bool AnimationPlayer::isFinished() const { return mFinished; }

//------------------------------------------------------------------------------
bool AnimationPlayer::showFrame(int index) {
  if (!mMovie || index < kFirstFrame || index >= mFrameCount)
    return false;
  setFinished(false);
  if (mMovie->currentFrameNumber() == index)
    return true;
  // With CacheAll, direct frame jumps are instantaneous in both directions.
  // A jump past the decodable part of a truncated file wraps to the first
  // frame and still reports success, so the frame reached is checked.
  if (!mMovie->jumpToFrame(index) || mMovie->currentFrameNumber() != index) {
    // Fallback for formats or states that do not support direct jumping:
    if (index < mMovie->currentFrameNumber() &&
        !mMovie->jumpToFrame(kFirstFrame)) {
      reportDecodeError(kFirstFrame);
      return false;
    }
    while (mMovie->currentFrameNumber() != index) {
      const int next = mMovie->currentFrameNumber() + 1;
      switch (stepForward()) {
      case FrameStep::Failed:
        reportDecodeError(next);
        return false;
      case FrameStep::Advanced:
        break;
      case FrameStep::Wrapped:
        // Frame index is beyond the decodable part of the file: the last
        // decodable frame stands in for it, as the end of the animation.
        endAnimationAt(next);
        index = mFrameCount - 1;
        break;
      }
    }
  }
  publishCurrentFrame();
  return true;
}

void AnimationPlayer::nextFrame() {
  if (!mMovie)
    return;
  const int last = mFrameCount - 1;
  showFrame(mMovie->currentFrameNumber() >= last
                ? kFirstFrame
                : mMovie->currentFrameNumber() + 1);
}

void AnimationPlayer::prevFrame() {
  if (!mMovie)
    return;
  showFrame(mMovie->currentFrameNumber() <= kFirstFrame
                ? mFrameCount - 1
                : mMovie->currentFrameNumber() - 1);
}

void AnimationPlayer::firstFrame() {
  if (mMovie)
    showFrame(kFirstFrame);
}

void AnimationPlayer::lastFrame() {
  if (mMovie)
    showFrame(mFrameCount - 1);
}

int AnimationPlayer::frameIndex() const {
  return mMovie && mFrame ? mMovie->currentFrameNumber() : kNoFrame;
}

int AnimationPlayer::frameCount() const {
  return mFrameCount;
}

QSize AnimationPlayer::frameSize() const {
  return mFrame ? mFrame->size() : QSize();
}

std::shared_ptr<const QImage> AnimationPlayer::currentFrame() const {
  return mFrame;
}

int AnimationPlayer::scheduledDelay() const {
  return mTimer.isActive() ? mTimer.interval() : kNoDelay;
}

//------------------------------------------------------------------------------
void AnimationPlayer::onTimer() {
  if (!mMovie || !mPlaying)
    return;
  if (!mReversed) {
    if (mMovie->currentFrameNumber() < mFrameCount - 1) {
      const int next = mMovie->currentFrameNumber() + 1;
      switch (stepForward()) {
      case FrameStep::Failed:
        reportDecodeError(next);
        pause();
        return;
      case FrameStep::Advanced:
        publishCurrentFrame();
        return;
      case FrameStep::Wrapped:
        // The file ends before its last declared frame (a truncated GIF):
        // its decodable frames are the whole animation, and QMovie is back at
        // the first one.
        endAnimationAt(next);
        if (mLoop) {
          publishCurrentFrame();
        } else {
          // Back on the last frame, which is still the one on screen.
          pause();
          if (showFrame(mFrameCount - 1))
            finishPlayback();
        }
        return;
      }
    }
    if (!mLoop) {
      finishPlayback();
      return;
    }
    if (!mMovie->jumpToFrame(kFirstFrame)) {
      reportDecodeError(kFirstFrame);
      pause();
      return;
    }
    publishCurrentFrame();
  } else {
    if (mMovie->currentFrameNumber() > kFirstFrame) {
      if (!showFrame(mMovie->currentFrameNumber() - 1)) {
        pause();
      }
      return;
    }
    if (!mLoop) {
      finishPlayback();
      return;
    }
    if (!showFrame(mFrameCount - 1)) {
      pause();
      return;
    }
  }
}

AnimationPlayer::FrameStep AnimationPlayer::stepForward() {
  const int previous = mMovie->currentFrameNumber();
  if (!mMovie->jumpToNextFrame())
    return FrameStep::Failed;
  return mMovie->currentFrameNumber() > previous ? FrameStep::Advanced
                                                 : FrameStep::Wrapped;
}

void AnimationPlayer::endAnimationAt(int frameCount) {
  if (frameCount >= mFrameCount)
    return;
  mFrameCount = frameCount;
  emit frameCountChanged();
}

void AnimationPlayer::finishPlayback() {
  setPlaying(false);
  setFinished(true);
  emit playbackFinished();
}

void AnimationPlayer::publishCurrentFrame() {
  QImage frame = mMovie->currentImage();
  if (frame.isNull()) {
    reportDecodeError(mMovie->currentFrameNumber());
    pause();
    return;
  }
  mFrame = std::make_shared<const QImage>(std::move(frame));
  // Scheduled first: receivers of frameReady() see the frame's delay.
  if (mPlaying)
    scheduleNextFrame();
  emit frameReady(mFrame, mMovie->currentFrameNumber());
  emit frameChanged();
}

void AnimationPlayer::scheduleNextFrame() {
  const int delay = mMovie->nextFrameDelay();
  if (qFuzzyCompare(mSpeed, 1.0)) {
    mTimer.start(delay);
  } else {
    const int baseDelay = delay > 0 ? delay : 100;
    const int effectiveDelay = std::max(1, static_cast<int>(std::round(baseDelay / mSpeed)));
    mTimer.start(effectiveDelay);
  }
}

void AnimationPlayer::setPlaying(bool playing) {
  if (mPlaying == playing)
    return;
  mPlaying = playing;
  if (!mPlaying)
    mTimer.stop();
  emit playingChanged();
}

void AnimationPlayer::setFinished(bool finished) {
  if (mFinished == finished)
    return;
  mFinished = finished;
  emit finishedChanged();
}

void AnimationPlayer::reportDecodeError(int frameIndex) {
  emit playbackError({.kind = AnimationPlaybackError::Kind::DecodeFrame,
                      .filePath = mMovie->fileName(),
                      .frameNumber = frameIndex + 1,
                      .reason = mMovie->lastErrorString()});
}
