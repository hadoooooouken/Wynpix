#pragma once

#include <QList>
#include <QObject>
#include <QThreadPool>

#include <functional>
#include <memory>

class Image;

// Builds the SDR copies of HDR images that ImageStatic::getImage() returns on
// a worker thread, so the first CPU use of an HDR image (copy, save, print,
// edits, ...) does not tone-map it on the GUI thread.
class SdrCopyPreparer : public QObject {
    Q_OBJECT
public:
    explicit SdrCopyPreparer(QObject *parent = nullptr);
    // Waits for the conversion in progress.
    ~SdrCopyPreparer() override;

    // True when getImage() of each image returns without converting. Only
    // static images have an SDR copy to build.
    [[nodiscard]] static bool isReady(const QList<std::shared_ptr<Image>> &images);
    // Builds the SDR copies the images miss, then calls onPrepared on this
    // object's thread; never once this object is destroyed.
    void prepare(QList<std::shared_ptr<Image>> images,
                 std::function<void()> onPrepared);

signals:
    // True while a preparation is pending.
    void busyChanged(bool busy);

private:
    void finish(const std::function<void()> &onPrepared);

    // One worker: the tone mapper spreads each conversion over the global
    // pool and waits for it, so it must not run there itself.
    QThreadPool mPool;
    int mPending = 0;
};
