#include "loaderrunnable.h"

#include <utility>

#include <QImageReader>

namespace {
constexpr int BitsPerByte = 8;
// Readers that do not report their pixel format decode to 32-bit ARGB/RGB.
constexpr int FallbackBitsPerPixel = 32;

// Decoded size from the file header, without decoding; nullopt when the
// reader cannot report the dimensions (the decoded image is checked instead).
std::optional<qint64> estimateDecodedBytes(const QString &path) {
    QImageReader reader(path);
    const QSize size = reader.size();
    if (!size.isValid() || size.isEmpty())
        return std::nullopt;
    const QImage::Format format = reader.imageFormat();
    const int bitsPerPixel =
        format == QImage::Format_Invalid
            ? FallbackBitsPerPixel
            : static_cast<int>(QImage::toPixelFormat(format).bitsPerPixel());
    return static_cast<qint64>(size.width()) * size.height() * bitsPerPixel /
           BitsPerByte;
}
} // namespace

void LoaderTaskNotifier::reportCompletion(ImageLoadCompletion completion) {
    emit taskCompleted(std::move(completion));
}

LoaderRunnable::LoaderRunnable(ImageLoadRequest request,
                               LoaderTaskNotifier &notifier)
    : request(std::move(request)), notifier(notifier) {
    completion.taskId = this->request.taskId;
    setAutoDelete(true);
}

LoaderRunnable::~LoaderRunnable() {
    notifier.reportCompletion(std::move(completion));
}

void LoaderRunnable::run() {
    if (request.decodedBytesLimit) {
        const std::optional<qint64> estimate =
            estimateDecodedBytes(request.path);
        if (estimate && *estimate > *request.decodedBytesLimit) {
            completion.status = ImageLoadCompletionStatus::OverMemoryBudget;
            return;
        }
    }

    completion.image =
        ImageFactory::createImage(request.path, request.decodeContext);

    // The header estimate is unavailable for some formats (PDF, DjVu) and
    // can differ from the decoded format; the decoded size decides.
    if (request.decodedBytesLimit && completion.image &&
        completion.image->decodedBytes() > *request.decodedBytesLimit) {
        completion.image.reset();
        completion.status = ImageLoadCompletionStatus::OverMemoryBudget;
        return;
    }

    // Warm the lazy DocumentInfo cache (exif / ComfyUI generation-info) here,
    // on the loader pool worker thread, instead of leaving the first, expensive
    // call to run synchronously on the GUI thread inside Core::guiSetImage().
    // getExifTags()/getGenerationInfo() are idempotent and cache their result
    // (exifLoaded/generationInfoLoaded), so this call absorbs the costly first
    // parse; the later call from guiSetImage() just hits the cache.
    // Skipped if the load was already cancelled or decode failed, since there
    // is nothing to read metadata from in that case.
    // Measured: ~0-1ms per file on characteristic ComfyUI PNGs (vs 90-150ms
    // decode), well under the threshold for needing a separate async pipeline.
    if (completion.image && !request.decodeContext.isCancellationRequested()) {
        completion.image->getExifTags();
        completion.image->getGenerationInfo();
    }

    completion.status = ImageLoadCompletionStatus::Finished;
}
