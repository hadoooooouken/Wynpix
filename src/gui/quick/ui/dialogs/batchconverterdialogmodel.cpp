#include "batchconverterdialogmodel.h"

#include <QDebug>
#include <QVariantMap>

#include <algorithm>
#include <cmath>
#include <iterator>

namespace {
using namespace BatchJobRules;
using namespace Qt::StringLiterals;

// Side of the queue's thumbnails (the widget dialog's 48 px labels).
constexpr int kThumbnailSize = 48;
// Range of the progress bar before the first batch (QProgressBar's default).
constexpr int kIdleProgressMaximum = 100;

constexpr int kAspectFitModeCount = static_cast<int>(AspectFitMode::Height) + 1;

template <typename T>
[[nodiscard]] bool isIndexOf(int index, const QList<T> &list) {
    return index >= 0 && index < list.size();
}
} // namespace

BatchConverterDialogModel::BatchConverterDialogModel(QObject *parent)
    : DialogSession(parent), mStatusText(readyText()) {
    // Whatever closed the dialog, the service goes away after it.
    connect(this, &DialogSession::finished, this, [this]() {
        detachService();
        closeMessage();
        // Cancel, Escape and the close button keep the list for the next time
        // the dialog opens (Add more already took it). It ends once a batch
        // was started, and Clear leaves nothing to keep.
        if (!mListKept && !mStarted && mQueue.rowCount() > 0) {
            mKeptFiles = mQueue.entries();
            setListKept(true);
        }
        mQueue.clear();
        mRunning = false;
        mCancelling = false;
        emit runChanged();
    });
}

BatchConverterDialogModel::~BatchConverterDialogModel() = default;

//--- constant choices --------------------------------------------------------
BatchQueueModel *BatchConverterDialogModel::queue() {
    return &mQueue;
}

QStringList BatchConverterDialogModel::formats() const {
    QStringList labels;
    for (const OutputFormat &format : outputFormats())
        labels.append(format.label);
    return labels;
}

QStringList BatchConverterDialogModel::filters() const {
    QStringList labels;
    for (const ScalingFilterOption &filter : scalingFilters())
        labels.append(filter.label);
    return labels;
}

QStringList BatchConverterDialogModel::commonSizes() const {
    QStringList labels;
    for (const CommonSize &size : BatchJobRules::commonSizes())
        labels.append(size.label);
    return labels;
}

QStringList BatchConverterDialogModel::rotations() const {
    QStringList labels;
    for (const RotationAngle angle : kRotations)
        labels.append(rotationLabel(angle));
    return labels;
}

QVariantList BatchConverterDialogModel::colorSliders() const {
    QVariantList sliders;
    for (const ColorSliderSpec &spec : BatchJobRules::colorSliders()) {
        sliders.append(QVariantMap{{u"label"_s, spec.label},
                                   {u"minimum"_s, spec.minimum},
                                   {u"maximum"_s, spec.maximum},
                                   {u"defaultValue"_s, spec.defaultValue},
                                   {u"step"_s, spec.step},
                                   {u"decimals"_s, spec.decimals},
                                   {u"suffix"_s, spec.suffix},
                                   {u"signedValue"_s, spec.signedValue}});
    }
    return sliders;
}

double BatchConverterDialogModel::minimumPercent() const {
    return kMinimumPercent;
}

double BatchConverterDialogModel::maximumPercent() const {
    return kMaximumPercent;
}

int BatchConverterDialogModel::percentDecimals() const {
    return kPercentDecimals;
}

double BatchConverterDialogModel::minimumMegapixels() const {
    return kMinimumMegapixels;
}

double BatchConverterDialogModel::maximumMegapixels() const {
    return kMaximumMegapixels;
}

int BatchConverterDialogModel::megapixelDecimals() const {
    return kMegapixelDecimals;
}

int BatchConverterDialogModel::minimumSide() const {
    return kMinimumSide;
}

int BatchConverterDialogModel::maximumSide() const {
    return kMaximumSide;
}

QString BatchConverterDialogModel::patternHelp() const {
    return BatchJobRules::patternHelp();
}

int BatchConverterDialogModel::thumbnailSize() const {
    return kThumbnailSize;
}

//--- draft -------------------------------------------------------------------
int BatchConverterDialogModel::formatIndex() const {
    return mFormatIndex;
}

bool BatchConverterDialogModel::qualityAvailable() const {
    return mQualityScale.kind != QualityKind::None;
}

int BatchConverterDialogModel::qualityMinimum() const {
    return mQualityScale.minimum;
}

int BatchConverterDialogModel::qualityMaximum() const {
    return mQualityScale.maximum;
}

int BatchConverterDialogModel::quality() const {
    return mDraft.job.quality;
}

QString BatchConverterDialogModel::qualityToolTip() const {
    return BatchJobRules::qualityToolTip(mQualityScale.kind);
}

bool BatchConverterDialogModel::resizeEnabled() const {
    return mDraft.job.doResize;
}

bool BatchConverterDialogModel::byPercent() const {
    return mDraft.job.resizeMode == ResizeMode::ByPercent;
}

bool BatchConverterDialogModel::byAbsoluteSize() const {
    return mDraft.job.resizeMode == ResizeMode::ByAbsoluteSize;
}

bool BatchConverterDialogModel::byMegapixels() const {
    return mDraft.job.resizeMode == ResizeMode::ByMegapixels;
}

double BatchConverterDialogModel::percent() const {
    return mDraft.job.resizePercent;
}

double BatchConverterDialogModel::targetMegapixels() const {
    return mDraft.job.targetMegapixels;
}

bool BatchConverterDialogModel::useBinaryMegapixel() const {
    return mDraft.job.useBinaryMegapixel;
}

int BatchConverterDialogModel::targetWidth() const {
    return mDraft.job.targetSize.width();
}

int BatchConverterDialogModel::targetHeight() const {
    return mDraft.job.targetSize.height();
}

int BatchConverterDialogModel::originalWidth() const {
    return mOriginalSize.width();
}

int BatchConverterDialogModel::originalHeight() const {
    return mOriginalSize.height();
}

bool BatchConverterDialogModel::keepAspectRatio() const {
    return mDraft.job.keepAspectRatio;
}

int BatchConverterDialogModel::aspectFitMode() const {
    return static_cast<int>(mDraft.job.aspectFitMode);
}

int BatchConverterDialogModel::commonSizeIndex() const {
    return mCommonSizeIndex;
}

int BatchConverterDialogModel::filterIndex() const {
    return mFilterIndex;
}

bool BatchConverterDialogModel::upscaylAvailable() const {
    return !mInput.upscaylModels.isEmpty();
}

bool BatchConverterDialogModel::useUpscayl() const {
    return mDraft.job.useUpscayl;
}

QStringList BatchConverterDialogModel::upscaylModels() const {
    return mInput.upscaylModels;
}

int BatchConverterDialogModel::upscaylModelIndex() const {
    return mUpscaylModelIndex;
}

int BatchConverterDialogModel::rotationIndex() const {
    return mRotationIndex;
}

bool BatchConverterDialogModel::flipHorizontal() const {
    return mDraft.job.flipHorizontal;
}

bool BatchConverterDialogModel::flipVertical() const {
    return mDraft.job.flipVertical;
}

bool BatchConverterDialogModel::colorEnabled() const {
    return mDraft.colorEnabled;
}

QList<double> BatchConverterDialogModel::colorValues() const {
    return {mDraft.colorValues.begin(), mDraft.colorValues.end()};
}

QString BatchConverterDialogModel::outputDirectory() const {
    return mDraft.job.outputDir;
}

QUrl BatchConverterDialogModel::outputDirectoryUrl() const {
    const QString directory = mDraft.job.outputDir.trimmed();
    return directory.isEmpty() ? QUrl() : QUrl::fromLocalFile(directory);
}

bool BatchConverterDialogModel::createSubfolder() const {
    return mDraft.job.createSubfolder;
}

QString BatchConverterDialogModel::pattern() const {
    return mDraft.job.pattern;
}

bool BatchConverterDialogModel::overwrite() const {
    return mDraft.job.overwrite;
}

//--- run ---------------------------------------------------------------------
bool BatchConverterDialogModel::isRunning() const {
    return mRunning;
}

bool BatchConverterDialogModel::isCancelling() const {
    return mCancelling;
}

int BatchConverterDialogModel::progressValue() const {
    return mProcessed;
}

int BatchConverterDialogModel::progressMaximum() const {
    return mProgressMaximum;
}

QString BatchConverterDialogModel::statusText() const {
    return mStatusText;
}

bool BatchConverterDialogModel::messageOpen() const {
    return mMessageOpen;
}

bool BatchConverterDialogModel::messageIsWarning() const {
    return mMessageIsWarning;
}

QString BatchConverterDialogModel::messageTitle() const {
    return mMessage.title;
}

QString BatchConverterDialogModel::messageText() const {
    return mMessage.text;
}

bool BatchConverterDialogModel::isListKept() const {
    return mListKept;
}

BatchConversionResult BatchConverterDialogModel::result() const {
    return {.conversionStarted = mStarted, .listKept = mListKept, .files = mKeptFiles};
}

BatchConverterPreferences BatchConverterDialogModel::preferencesToStore() const {
    return {.useUpscayl = mDraft.job.useUpscayl,
            .upscaylModel = selectedUpscaylModel(),
            .saved = savedSettingsOf(mDraft, mOriginalSize)};
}

QString BatchConverterDialogModel::selectedUpscaylModel() const {
    return isIndexOf(mUpscaylModelIndex, mInput.upscaylModels)
               ? mInput.upscaylModels[mUpscaylModelIndex]
               : QString();
}

BatchJob BatchConverterDialogModel::job() const {
    BatchJobDraft draft = mDraft;
    draft.job.upscaylModel = selectedUpscaylModel();
    return finalJob(draft);
}

//--- request -----------------------------------------------------------------
bool BatchConverterDialogModel::start(const BatchConverterDialogInput &input,
                                      BatchConversionService &service) {
    if (!canStart("batch converter"))
        return false;

    mInput = input;
    mOriginalSize = input.originalSize.isValid() && !input.originalSize.isEmpty()
                        ? input.originalSize
                        : fallbackOriginalSize();

    const QList<OutputFormat> formatList = outputFormats();
    const QList<ScalingFilterOption> filterList = scalingFilters();
    mDraft = {};
    BatchJob &job = mDraft.job;
    job.format = formatList.first().extension;
    job.resizePercent = kDefaultPercent;
    job.targetMegapixels = megapixelsForSize(mOriginalSize, job.useBinaryMegapixel);
    job.targetSize = mOriginalSize;
    mFilterIndex = defaultScalingFilterIndex();
    job.scalingFilter = filterList[mFilterIndex].filter;
    job.useUpscayl = upscaylAvailable() && input.useUpscayl;
    const qsizetype storedModel = input.upscaylModels.indexOf(input.upscaylModel);
    mUpscaylModelIndex = upscaylAvailable() ? static_cast<int>(std::max<qsizetype>(storedModel, 0))
                                            : -1;
    job.pattern = defaultPattern();
    job.outputDir = initialOutputDirectory(
        input.defaultOutputDirectory, input.files.isEmpty() ? QString() : input.files.first().path);
    mFormatIndex = 0;
    mCommonSizeIndex = 0;
    mRotationIndex = 0;
    applyFormatQuality();
    if (input.saved)
        applySavedSettings(*input.saved);

    mRunning = false;
    mCancelling = false;
    mStarted = false;
    mKeptFiles.clear();
    setListKept(false);
    mProcessed = 0;
    mProgressMaximum = kIdleProgressMaximum;
    mStatusText = readyText();
    mMessageOpen = false;
    mMessage = {};

    mQueue.reset(input.files);
    connectService(service);
    const int thumbnailExtent =
        static_cast<int>(std::lround(kThumbnailSize * std::max(input.devicePixelRatio, 1.0)));
    service.requestThumbnails(mQueue.paths(), thumbnailExtent);
    service.requestAnimationCheck(mQueue.paths());

    emit settingsChanged();
    emit runChanged();
    emit messageChanged();
    open();
    return true;
}

void BatchConverterDialogModel::connectService(BatchConversionService &service) {
    detachService();
    mService = &service;
    connect(&service, &BatchConversionService::progressUpdated, this,
            &BatchConverterDialogModel::onProgressUpdated);
    connect(&service, &BatchConversionService::finished, this,
            &BatchConverterDialogModel::onFinished);
    connect(&service, &BatchConversionService::cancelled, this,
            &BatchConverterDialogModel::onCancelled);
    connect(&service, &BatchConversionService::startFailed, this,
            &BatchConverterDialogModel::onStartFailed);
    connect(&service, &BatchConversionService::thumbnailReady, &mQueue,
            &BatchQueueModel::setThumbnail);
    connect(&service, &BatchConversionService::animationFound, &mQueue,
            &BatchQueueModel::setAnimated);
}

void BatchConverterDialogModel::detachService() {
    if (!mService)
        return;
    disconnect(mService, nullptr, this, nullptr);
    disconnect(mService, nullptr, &mQueue, nullptr);
    if (mRunning && !mCancelling)
        mService->cancel();
    mService = nullptr;
}

//--- editing -----------------------------------------------------------------
void BatchConverterDialogModel::setFormatIndex(int index) {
    const QList<OutputFormat> formatList = outputFormats();
    if (!isIndexOf(index, formatList) || index == mFormatIndex)
        return;
    mFormatIndex = index;
    mDraft.job.format = formatList[index].extension;
    applyFormatQuality();
    emit settingsChanged();
}

void BatchConverterDialogModel::applyFormatQuality() {
    mQualityScale = qualityScaleFor(mDraft.job.format);
    const int quality = defaultQualityFor(mDraft.job.format, mInput.saveQualities);
    // A format without a quality setting keeps the previous value.
    if (mQualityScale.kind != QualityKind::None)
        mDraft.job.quality = std::clamp(quality, mQualityScale.minimum, mQualityScale.maximum);
}

void BatchConverterDialogModel::applySavedSettings(const SavedSettings &stored) {
    const SavedSettings saved = sanitized(stored);
    BatchJob &job = mDraft.job;

    const QList<OutputFormat> formatList = outputFormats();
    const auto format = std::ranges::find(formatList, saved.format, &OutputFormat::extension);
    if (format != formatList.end()) {
        mFormatIndex = static_cast<int>(std::distance(formatList.begin(), format));
        job.format = format->extension;
        applyFormatQuality();
        if (saved.quality)
            job.quality = *saved.quality;
    }

    // The fields are restored as the setters would have left them, in the
    // order the resize section depends on them.
    job.doResize = saved.resizeEnabled;
    job.resizeMode = saved.resizeMode;
    job.resizePercent = saved.resizePercent;
    job.useBinaryMegapixel = saved.useBinaryMegapixel;
    job.targetMegapixels = saved.targetMegapixels.value_or(
        megapixelsForSize(mOriginalSize, job.useBinaryMegapixel));
    job.keepAspectRatio = saved.keepAspectRatio;
    job.aspectFitMode = saved.aspectFitMode;
    switch (job.resizeMode) {
    case ResizeMode::ByPercent:
        // The same percent applies to both sides of every file.
        job.keepAspectRatio = true;
        job.targetSize = percentTarget(mOriginalSize, job.resizePercent);
        break;
    case ResizeMode::ByAbsoluteSize:
        if (saved.targetSize.isValid())
            job.targetSize = saved.targetSize;
        mCommonSizeIndex = commonSizeIndexFor(job.targetSize);
        break;
    case ResizeMode::ByMegapixels:
        break;
    }

    if (saved.scalingFilter) {
        const QList<ScalingFilterOption> filterList = scalingFilters();
        const auto filter = std::ranges::find(filterList, *saved.scalingFilter,
                                              &ScalingFilterOption::filter);
        if (filter != filterList.end()) {
            mFilterIndex = static_cast<int>(std::distance(filterList.begin(), filter));
            job.scalingFilter = filter->filter;
        }
    }

    if (!saved.pattern.isEmpty())
        job.pattern = saved.pattern;
    job.createSubfolder = saved.createSubfolder;
}

void BatchConverterDialogModel::setQuality(int quality) {
    if (!qualityAvailable())
        return;
    mDraft.job.quality = std::clamp(quality, mQualityScale.minimum, mQualityScale.maximum);
    emit settingsChanged();
}

void BatchConverterDialogModel::applySize(QSize target) {
    mDraft.job.targetSize = target;
    emit settingsChanged();
}

void BatchConverterDialogModel::setResizeEnabled(bool enabled) {
    if (mDraft.job.doResize == enabled)
        return;
    mDraft.job.doResize = enabled;
    if (!enabled) {
        emit settingsChanged();
        return;
    }
    // Enabling the section applies its current mode again, as the widget
    // dialog's radio buttons did.
    if (mDraft.job.resizeMode == ResizeMode::ByPercent)
        setByPercent(true);
    else if (mDraft.job.resizeMode == ResizeMode::ByAbsoluteSize)
        setByPercent(false);
    else
        emit settingsChanged();
}

void BatchConverterDialogModel::setByPercent(bool byPercent) {
    BatchJob &job = mDraft.job;
    if (byPercent) {
        job.resizeMode = ResizeMode::ByPercent;
        // The same percent applies to both sides of every file.
        job.keepAspectRatio = true;
        applySize(percentTarget(mOriginalSize, job.resizePercent));
    } else {
        job.resizeMode = ResizeMode::ByAbsoluteSize;
        applySize(widthEdited(mOriginalSize, job.targetSize, job.targetSize.width(),
                              job.keepAspectRatio));
    }
}

void BatchConverterDialogModel::setByMegapixels(bool byMegapixels) {
    if (!byMegapixels) {
        if (mDraft.job.resizeMode == ResizeMode::ByMegapixels)
            setByPercent(false);
        return;
    }
    mDraft.job.resizeMode = ResizeMode::ByMegapixels;
    emit settingsChanged();
}

void BatchConverterDialogModel::setPercent(double percent) {
    mDraft.job.resizePercent = std::clamp(percent, kMinimumPercent, kMaximumPercent);
    applySize(percentTarget(mOriginalSize, mDraft.job.resizePercent));
}

void BatchConverterDialogModel::setTargetMegapixels(double megapixels) {
    mDraft.job.targetMegapixels =
        std::clamp(megapixels, kMinimumMegapixels, kMaximumMegapixels);
    emit settingsChanged();
}

void BatchConverterDialogModel::setUseBinaryMegapixel(bool useBinaryMegapixel) {
    mDraft.job.useBinaryMegapixel = useBinaryMegapixel;
    emit settingsChanged();
}

void BatchConverterDialogModel::setTargetWidth(int width) {
    const BatchJob &job = mDraft.job;
    if (job.resizeMode != ResizeMode::ByAbsoluteSize)
        return;
    applySize(widthEdited(mOriginalSize, job.targetSize,
                          std::clamp(width, kMinimumSide, kMaximumSide), job.keepAspectRatio));
}

void BatchConverterDialogModel::setTargetHeight(int height) {
    const BatchJob &job = mDraft.job;
    if (job.resizeMode != ResizeMode::ByAbsoluteSize)
        return;
    applySize(heightEdited(mOriginalSize, job.targetSize,
                           std::clamp(height, kMinimumSide, kMaximumSide), job.keepAspectRatio));
}

void BatchConverterDialogModel::setKeepAspectRatio(bool keep) {
    if (mDraft.job.resizeMode != ResizeMode::ByAbsoluteSize)
        return;
    mDraft.job.keepAspectRatio = keep;
    emit settingsChanged();
}

void BatchConverterDialogModel::setAspectFitMode(int mode) {
    if (mode < 0 || mode >= kAspectFitModeCount) {
        qWarning() << "Batch converter: unknown aspect fit mode" << mode;
        return;
    }
    mDraft.job.aspectFitMode = static_cast<AspectFitMode>(mode);
    emit settingsChanged();
}

void BatchConverterDialogModel::selectCommonSize(int index) {
    const QList<CommonSize> sizes = BatchJobRules::commonSizes();
    if (!isIndexOf(index, sizes))
        return;
    mCommonSizeIndex = index;
    // A common size is a bounding box in pixels; the aspect ratio is applied
    // per file during the conversion.
    if (sizes[index].size.isValid())
        setByPercent(false);
    applySize(sizes[index].size.isValid() ? sizes[index].size : mOriginalSize);
}

void BatchConverterDialogModel::resetSize() {
    mCommonSizeIndex = 0;
    mDraft.job.resizePercent = kDefaultPercent;
    mDraft.job.targetMegapixels =
        megapixelsForSize(mOriginalSize, mDraft.job.useBinaryMegapixel);
    applySize(mOriginalSize);
}

void BatchConverterDialogModel::setFilterIndex(int index) {
    const QList<ScalingFilterOption> filterList = scalingFilters();
    if (!isIndexOf(index, filterList))
        return;
    mFilterIndex = index;
    mDraft.job.scalingFilter = filterList[index].filter;
    emit settingsChanged();
}

void BatchConverterDialogModel::setUseUpscayl(bool use) {
    if (!upscaylAvailable())
        return;
    mDraft.job.useUpscayl = use;
    emit settingsChanged();
}

void BatchConverterDialogModel::setUpscaylModelIndex(int index) {
    if (!isIndexOf(index, mInput.upscaylModels))
        return;
    mUpscaylModelIndex = index;
    emit settingsChanged();
}

void BatchConverterDialogModel::setRotationIndex(int index) {
    if (index < 0 || index >= static_cast<int>(kRotations.size()))
        return;
    mRotationIndex = index;
    mDraft.job.rotation = kRotations[static_cast<std::size_t>(index)];
    emit settingsChanged();
}

void BatchConverterDialogModel::setFlipHorizontal(bool flip) {
    mDraft.job.flipHorizontal = flip;
    emit settingsChanged();
}

void BatchConverterDialogModel::setFlipVertical(bool flip) {
    mDraft.job.flipVertical = flip;
    emit settingsChanged();
}

void BatchConverterDialogModel::setColorEnabled(bool enabled) {
    mDraft.colorEnabled = enabled;
    emit settingsChanged();
}

void BatchConverterDialogModel::setColorValue(int slider, double value) {
    const QList<ColorSliderSpec> sliders = BatchJobRules::colorSliders();
    if (!isIndexOf(slider, sliders)) {
        qWarning() << "Batch converter: unknown colour slider" << slider;
        return;
    }
    const ColorSliderSpec &spec = sliders[slider];
    mDraft.colorValues[static_cast<std::size_t>(slider)] =
        std::clamp(value, spec.minimum, spec.maximum);
    emit settingsChanged();
}

void BatchConverterDialogModel::resetColorValues() {
    mDraft.colorValues = defaultColorValues();
    emit settingsChanged();
}

void BatchConverterDialogModel::setOutputDirectory(const QString &directory) {
    mDraft.job.outputDir = directory;
    emit settingsChanged();
}

void BatchConverterDialogModel::setOutputDirectoryUrl(const QUrl &url) {
    if (!url.isLocalFile()) {
        qWarning() << "Batch converter: the output folder must be local, not" << url;
        return;
    }
    setOutputDirectory(url.toLocalFile());
}

void BatchConverterDialogModel::setCreateSubfolder(bool create) {
    mDraft.job.createSubfolder = create;
    emit settingsChanged();
}

void BatchConverterDialogModel::setPattern(const QString &pattern) {
    mDraft.job.pattern = pattern;
    emit settingsChanged();
}

void BatchConverterDialogModel::setOverwrite(bool overwrite) {
    mDraft.job.overwrite = overwrite;
    emit settingsChanged();
}

//--- run ---------------------------------------------------------------------
void BatchConverterDialogModel::convert() {
    if (!isOpen() || mRunning || mCancelling)
        return;

    const StartCheck check{.outputDirectory = mDraft.job.outputDir,
                           .pattern = mDraft.job.pattern,
                           .selectedCount = mQueue.selectedCount(),
                           .resize = mDraft.job.doResize,
                           .useUpscayl = mDraft.job.useUpscayl,
                           .targetSize = mDraft.job.resizeMode == ResizeMode::ByMegapixels
                                             ? megapixelTarget(mOriginalSize,
                                                               mDraft.job.targetMegapixels,
                                                               mDraft.job.useBinaryMegapixel)
                                             : mDraft.job.targetSize};
    const StartProblem problem = checkStart(check);
    if (problem != StartProblem::None) {
        showMessage(startProblemMessage(problem, check), true);
        return;
    }
    if (!mService) {
        qWarning() << "Batch converter: no conversion service; the batch was not started";
        onStartFailed(QString());
        return;
    }

    const BatchJob batchJob = job();
    const QList<int> rows = mQueue.selectedRows();
    for (const int row : rows)
        mQueue.setItemState(row, BatchItemState::Pending, QString());
    mRunning = true;
    mStarted = true;
    mProcessed = 0;
    mProgressMaximum = static_cast<int>(rows.size());
    mStatusText = processingText();
    emit runChanged();
    mService->start(mQueue.paths(), rows, batchJob);
}

void BatchConverterDialogModel::stopOrClose() {
    if (mCancelling)
        return;
    if (!mRunning) {
        reject();
        return;
    }
    mCancelling = true;
    setStatusText(stoppingText());
    if (mService)
        mService->cancel();
}

void BatchConverterDialogModel::reject() {
    conclude();
}

void BatchConverterDialogModel::addMore() {
    if (!isOpen() || mRunning || mCancelling)
        return;
    mKeptFiles = mQueue.entries();
    setListKept(!mKeptFiles.isEmpty());
    conclude();
}

void BatchConverterDialogModel::clearList() {
    if (!isOpen() || mRunning || mCancelling)
        return;
    mQueue.clear();
    // Whatever a finished batch left behind goes with the files.
    mProcessed = 0;
    mProgressMaximum = kIdleProgressMaximum;
    mStatusText = readyText();
    closeMessage();
    emit runChanged();
}

void BatchConverterDialogModel::setListKept(bool kept) {
    if (mListKept == kept)
        return;
    mListKept = kept;
    emit listKeptChanged();
}

void BatchConverterDialogModel::showMessage(const Message &message, bool warning) {
    mMessage = message;
    mMessageIsWarning = warning;
    mMessageOpen = true;
    emit messageChanged();
}

void BatchConverterDialogModel::closeMessage() {
    if (!mMessageOpen)
        return;
    mMessageOpen = false;
    emit messageChanged();
}

void BatchConverterDialogModel::dismissMessage() {
    closeMessage();
}

void BatchConverterDialogModel::setStatusText(const QString &text) {
    mStatusText = text;
    emit runChanged();
}

void BatchConverterDialogModel::onProgressUpdated(int index, BatchItemState state,
                                                  const QString &details) {
    mQueue.setItemState(index, state, details);
    if (state == BatchItemState::Processing)
        return;
    ++mProcessed;
    setStatusText(processedText(mProcessed, mProgressMaximum));
}

void BatchConverterDialogModel::onFinished(int succeeded, int failed, int total) {
    mRunning = false;
    setStatusText(finishedText(succeeded, failed));
    showMessage(completedMessage(succeeded, failed, total), false);
}

void BatchConverterDialogModel::onCancelled(int succeeded, int failed, int total) {
    Q_UNUSED(total)
    for (int row = 0; row < mQueue.rowCount(); ++row) {
        if (mQueue.data(mQueue.index(row), BatchQueueModel::StateRole).toInt() ==
            static_cast<int>(BatchItemState::Processing)) {
            mQueue.setItemState(row, BatchItemState::Stopped, QString());
            ++mProcessed;
        }
    }
    mRunning = false;
    mCancelling = false;
    setStatusText(stoppedText(succeeded, failed));
}

void BatchConverterDialogModel::onStartFailed(const QString &reason) {
    mRunning = false;
    setStatusText(abortedText());
    if (!reason.isEmpty())
        showMessage(startFailedMessage(reason), true);
}
