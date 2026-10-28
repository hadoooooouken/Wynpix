#pragma once

#include <QObject>
#include <QString>
#include <QThreadPool>

#include <functional>
#include <memory>

class Image;

// Readies an image for a CPU action (copy, save, print, edits, ...) off the
// GUI thread: decodes it when it is not decoded yet and builds the SDR copy
// of an HDR image that ImageStatic::getImage() returns.
class ImagePreparer : public QObject {
    Q_OBJECT
public:
    // Called on the GUI thread with the prepared image; null when it could
    // not be decoded.
    using Prepared = std::function<void(std::shared_ptr<Image> image)>;

    explicit ImagePreparer(QObject *parent = nullptr);
    // Waits for the preparation in progress.
    ~ImagePreparer() override;

    // True when image is decoded and its getImage() returns without
    // converting. Only static images have an SDR copy to build.
    [[nodiscard]] static bool isReady(const std::shared_ptr<Image> &image);
    // Decodes path when image is null, builds the SDR copy, then calls
    // onPrepared on this object's thread; never once this object is
    // destroyed.
    void prepare(QString path, std::shared_ptr<Image> image,
                 Prepared onPrepared);

signals:
    // True while a preparation is pending.
    void busyChanged(bool busy);

private:
    void finish(const Prepared &onPrepared, std::shared_ptr<Image> image);

    // One worker, so an action over a selection holds one decoded image at a
    // time. The tone mapper spreads each conversion over the global pool and
    // waits for it, so it must not run there itself.
    QThreadPool mPool;
    int mPending = 0;
};
