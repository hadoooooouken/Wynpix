#pragma once

#include <QRunnable>
#include <QImage>
#include <QRect>
#include <QString>
#include <QSize>
#include <memory>
#include <atomic>
#include <optional>
#include "utils/hdrtonemapper.h"

class Upscaler;

struct UpscalerTaskParams {
    // The visible crop as decoded; the runnable converts an HDR crop to SDR
    // and caps its size before the inference.
    QImage croppedImage;
    // Set when croppedImage is HDR: the SDR conversion to apply to it.
    std::optional<HdrToneMapParams> sdrConversion;
    // The source area of croppedImage; adjusted to what the capped crop
    // covers.
    QRect origCrop;
    QString path;
    QSize targetSize;
    QString modelName;
    uint64_t generation = 0;
};

class UpscalerRunnable : public QRunnable {
public:
    UpscalerRunnable(Upscaler *upscaler, const UpscalerTaskParams &params, std::shared_ptr<std::atomic<bool>> abortFlag);
    void run() override;

private:
    // Largest crop side sent to the model, to prevent Vulkan/GPU OOM.
    static constexpr int kMaxCropDimension = 1280;

    // Converts an HDR crop to SDR and caps the crop size, adjusting origCrop.
    // False when the SDR conversion fails.
    bool prepareCrop();

    Upscaler *upscaler;
    UpscalerTaskParams params;
    std::shared_ptr<std::atomic<bool>> abortFlag;
};
