#pragma once

#include <QObject>
#include <QPointer>
#include <QString>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

#include "components/animationplayer/animationplayer.h"

// Presentation state and actions of the animation playback HUD (AnimationPlaybackOverlay.qml).
//
// Owned by OverlayCoordinator. GUI thread only.
class AnimationPlaybackController final : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by OverlayCoordinator")
    Q_PROPERTY(bool visible READ isVisible NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool shown READ isShown NOTIFY shownChanged FINAL)
    Q_PROPERTY(bool playing READ isPlaying NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool loop READ isLoop NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool reversed READ isReversed NOTIFY stateChanged FINAL)
    Q_PROPERTY(int frameIndex READ frameIndex NOTIFY frameChanged FINAL)
    Q_PROPERTY(int frameCount READ frameCount NOTIFY stateChanged FINAL)
    Q_PROPERTY(qreal speed READ speed NOTIFY stateChanged FINAL)
    Q_PROPERTY(QString speedText READ speedText NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool rememberSettings READ isRememberSettings NOTIFY stateChanged FINAL)

public:
    static constexpr int kHideTimeoutMs = 2000;

    explicit AnimationPlaybackController(QObject *parent = nullptr);

    void attachPlayer(AnimationPlayer *player);

    [[nodiscard]] bool isVisible() const;
    [[nodiscard]] bool isShown() const;
    [[nodiscard]] bool isPlaying() const;
    [[nodiscard]] bool isLoop() const;
    [[nodiscard]] bool isReversed() const;
    [[nodiscard]] int frameIndex() const;
    [[nodiscard]] int frameCount() const;
    [[nodiscard]] qreal speed() const;
    [[nodiscard]] QString speedText() const;
    [[nodiscard]] bool isRememberSettings() const;

    void setFolderViewActive(bool active);
    void setEnabled(bool enabled);
    void setPanelAtTop(bool atTop);
    void pointerMoved();

    Q_INVOKABLE void setHovered(bool hovered);
    Q_INVOKABLE void togglePlayPause();
    Q_INVOKABLE void nextFrame();
    Q_INVOKABLE void prevFrame();
    Q_INVOKABLE void firstFrame();
    Q_INVOKABLE void lastFrame();
    Q_INVOKABLE void seekToFrame(int index);
    Q_INVOKABLE void setSpeed(qreal speed);
    Q_INVOKABLE void toggleLoop();
    Q_INVOKABLE void toggleReverse();
    Q_INVOKABLE void setRememberSettings(bool remember);
    Q_INVOKABLE void onScrubbingStarted();
    Q_INVOKABLE void onScrubbingFinished();

signals:
    void stateChanged();
    void frameChanged();
    void shownChanged();

private:
    void setShown(bool shown);
    void show();
    void updateVisibilityState();
    void updateTimer();

    QPointer<AnimationPlayer> mPlayer;
    QTimer mHideTimer;
    bool mEnabled = true;
    bool mFolderViewActive = false;
    bool mPanelAtTop = false;
    bool mShown = false;
    bool mHovered = false;
    bool mScrubbing = false;
    bool mWasPlayingBeforeScrub = false;
};
