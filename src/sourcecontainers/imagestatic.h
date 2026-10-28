#pragma once

#include <QHash>
#include <QImage>
#include <QImageWriter>
#include <QMutex>
#include <QSemaphore>
#include <QCryptographicHash>
#include "image.h"
#include "sourcecontainers/decodedpixels.h"
#include "utils/decodecontext.h"
#include "utils/imagelib.h"
#include <settings.h>
#include <QIcon>

// What an edit changed. Orientation covers rotating and flipping; Content
// covers every other edit (crop, resize, colour adjustments, ...).
enum class ImageEditKind { Orientation, Content };

class ImageStatic : public Image {
public:
    explicit ImageStatic(QString path, DecodeContext context = {});
    explicit ImageStatic(std::unique_ptr<DocumentInfo> info,
                         DecodeContext context = {});
    ~ImageStatic();

    std::shared_ptr<const QImage> getImage();
    // True when getImage() tone-maps the HDR source first.
    [[nodiscard]] bool needsSdrConversion();
    // Builds the SDR copy of the unedited pixels that getImage() returns.
    // Thread safe; ImagePreparer runs it on its worker.
    void prepareSdrCopy();
    // The decoded image (HDR stays HDR); the viewer tone maps and colour
    // manages it itself.
    std::shared_ptr<const QImage> getDisplayImage() override;
    std::shared_ptr<const QImage> getDecodedImage() override;
    // Changes whenever the effective image content is replaced or discarded.
    quint64 contentRevision() const noexcept;

    int height();
    int width();
    QSize size();

    bool setEditedImage(std::unique_ptr<const QImage> imageEditedNew,
                        ImageEditKind kind);
    // True while the image is edited and every edit since it was loaded or
    // last discarded was a rotation or a flip.
    [[nodiscard]] bool hasOnlyOrientationEdits() const noexcept;
    bool discardEditedImage();
    void commitEdits();

    static int pageOverrideForPath(const QString &path);
    static void setPageOverrideForPath(const QString &path, int pageIndex);
    int frameCount() const override;
    [[nodiscard]] qint64 decodedBytes() const override;
    int pageIndex() const noexcept;

public slots:
    void crop(QRect newRect);

private:
    void load();
    void loadPdf();
    void loadDjvu();
    // The HDR -> SDR conversion the current settings select.
    [[nodiscard]] static HdrToneMapParams sdrConversionParams();
    void setDecoded(std::shared_ptr<const QImage> decoded);

    DecodeContext mDecodeContext;
    // The unedited pixels; getImage() returns their SDR copy until edited.
    DecodedPixels pixels;
    std::shared_ptr<const QImage> imageEdited;
    quint64 mContentRevision = 0;
    bool mOrientationOnlyEdits = false;
    void clearEditedImageState() noexcept;
    void loadGeneric();
    void loadICO();
    int mPageCount = 1;
    int mPageIndex = 0;
    static QHash<QString, int> pageOverrides;
    static QMutex pageOverridesMutex;
};
