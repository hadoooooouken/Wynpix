#include "animationplaybackcontroller.h"

AnimationPlaybackController::AnimationPlaybackController(QObject *parent)
    : QObject(parent) {
    mHideTimer.setSingleShot(true);
    mHideTimer.setInterval(kHideTimeoutMs);
    connect(&mHideTimer, &QTimer::timeout, this, [this]() { setShown(false); });
}

void AnimationPlaybackController::attachPlayer(AnimationPlayer *player) {
    if (mPlayer == player)
        return;
    if (mPlayer)
        disconnect(mPlayer, nullptr, this, nullptr);
    mPlayer = player;
    if (mPlayer) {
        auto onPlayerState = [this]() {
            emit stateChanged();
            emit frameChanged();
            updateVisibilityState();
            updateTimer();
        };
        connect(mPlayer, &AnimationPlayer::frameChanged, this, &AnimationPlaybackController::frameChanged);
        connect(mPlayer, &AnimationPlayer::frameCountChanged, this, onPlayerState);
        connect(mPlayer, &AnimationPlayer::playingChanged, this, onPlayerState);
        connect(mPlayer, &AnimationPlayer::loopChanged, this, [this]() { emit stateChanged(); });
        connect(mPlayer, &AnimationPlayer::speedChanged, this, [this]() { emit stateChanged(); });
        connect(mPlayer, &AnimationPlayer::reversedChanged, this, [this]() { emit stateChanged(); });
        connect(mPlayer, &AnimationPlayer::rememberSettingsChanged, this, [this]() { emit stateChanged(); });
    }
    emit stateChanged();
    emit frameChanged();
    updateVisibilityState();
}

bool AnimationPlaybackController::isVisible() const {
    return !mFolderViewActive && !mPanelAtTop && mPlayer && mPlayer->frameCount() > 1;
}

bool AnimationPlaybackController::isShown() const {
    return mShown;
}

bool AnimationPlaybackController::isPlaying() const {
    return mPlayer ? mPlayer->isPlaying() : false;
}

bool AnimationPlaybackController::isLoop() const {
    return mPlayer ? mPlayer->loops() : false;
}

bool AnimationPlaybackController::isReversed() const {
    return mPlayer ? mPlayer->isReversed() : false;
}

int AnimationPlaybackController::frameIndex() const {
    return mPlayer ? mPlayer->frameIndex() : 0;
}

int AnimationPlaybackController::frameCount() const {
    return mPlayer ? mPlayer->frameCount() : 0;
}

qreal AnimationPlaybackController::speed() const {
    return mPlayer ? mPlayer->speed() : 1.0;
}

QString AnimationPlaybackController::speedText() const {
    return QString::number(speed(), 'g', 3) + u"x";
}

bool AnimationPlaybackController::isRememberSettings() const {
    return mPlayer ? mPlayer->rememberSettings() : false;
}

void AnimationPlaybackController::setFolderViewActive(bool active) {
    if (mFolderViewActive == active)
        return;
    mFolderViewActive = active;
    emit stateChanged();
    updateVisibilityState();
}

void AnimationPlaybackController::setPanelAtTop(bool atTop) {
    if (mPanelAtTop == atTop)
        return;
    mPanelAtTop = atTop;
    emit stateChanged();
    updateVisibilityState();
}

void AnimationPlaybackController::pointerMoved() {
    if (!isVisible())
        return;
    show();
}

void AnimationPlaybackController::setHovered(bool hovered) {
    if (mHovered == hovered)
        return;
    mHovered = hovered;
    if (!isVisible())
        return;
    if (hovered) {
        setShown(true);
    }
    updateTimer();
}

void AnimationPlaybackController::togglePlayPause() {
    if (mPlayer)
        mPlayer->togglePlayPause();
}

void AnimationPlaybackController::play() {
    if (mPlayer)
        mPlayer->play();
}

void AnimationPlaybackController::pause() {
    if (mPlayer)
        mPlayer->pause();
}

void AnimationPlaybackController::nextFrame() {
    if (mPlayer)
        mPlayer->nextFrame();
}

void AnimationPlaybackController::prevFrame() {
    if (mPlayer)
        mPlayer->prevFrame();
}

void AnimationPlaybackController::firstFrame() {
    if (mPlayer)
        mPlayer->firstFrame();
}

void AnimationPlaybackController::lastFrame() {
    if (mPlayer)
        mPlayer->lastFrame();
}

void AnimationPlaybackController::seekToFrame(int index) {
    if (mPlayer)
        mPlayer->showFrame(index);
}

void AnimationPlaybackController::setSpeed(qreal speed) {
    if (mPlayer)
        mPlayer->setSpeed(speed);
}

void AnimationPlaybackController::toggleLoop() {
    if (mPlayer)
        mPlayer->setLoop(!mPlayer->loops());
}

void AnimationPlaybackController::toggleReverse() {
    if (mPlayer)
        mPlayer->setReversed(!mPlayer->isReversed());
}

void AnimationPlaybackController::setRememberSettings(bool remember) {
    if (mPlayer)
        mPlayer->setRememberSettings(remember);
}

void AnimationPlaybackController::onScrubbingStarted() {
    mScrubbing = true;
    if (mPlayer) {
        mWasPlayingBeforeScrub = mPlayer->isPlaying();
        mPlayer->pause();
    }
    setShown(true);
    updateTimer();
}

void AnimationPlaybackController::onScrubbingFinished() {
    mScrubbing = false;
    if (mPlayer && mWasPlayingBeforeScrub) {
        mPlayer->play();
    }
    updateTimer();
}

void AnimationPlaybackController::setShown(bool shown) {
    if (mShown == shown)
        return;
    mShown = shown;
    emit shownChanged();
}

void AnimationPlaybackController::show() {
    setShown(true);
    updateTimer();
}

void AnimationPlaybackController::updateVisibilityState() {
    if (isVisible()) {
        show();
    } else {
        mHideTimer.stop();
        setShown(false);
    }
}

void AnimationPlaybackController::updateTimer() {
    if (!isVisible() || !mShown) {
        mHideTimer.stop();
        return;
    }
    if (!isPlaying() || mHovered || mScrubbing) {
        mHideTimer.stop();
    } else {
        mHideTimer.start();
    }
}
