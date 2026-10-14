#include "upscalerrunnable.h"
#include "upscaler.h"
#include <QCoreApplication>
#include <QDebug>
#include <QMetaObject>
#include <algorithm>

UpscalerRunnable::UpscalerRunnable(Upscaler *upscaler, const UpscalerTaskParams &params, std::shared_ptr<std::atomic<bool>> abortFlag)
    : upscaler(upscaler), params(params), abortFlag(abortFlag) {}

void UpscalerRunnable::run() {
    if (upscaler->isRequestStale(params.generation)) {
        QMetaObject::invokeMethod(upscaler, "onTaskAborted", Qt::QueuedConnection,
                                  Q_ARG(QString, params.path), Q_ARG(QSize, params.targetSize), Q_ARG(uint64_t, params.generation));
        return;
    }

    if (!prepareCrop()) {
        QMetaObject::invokeMethod(upscaler, "onTaskFailed", Qt::QueuedConnection,
                                  Q_ARG(QString, QCoreApplication::translate("Upscaler", "AI Upscaling processing failed")),
                                  Q_ARG(uint64_t, params.generation));
        return;
    }

    const QString appDir = QCoreApplication::applicationDirPath();
    const UpscaylInferenceResult inferenceResult =
        UpscaylScaler::getInstance()->upscale(
            UpscaylInferenceRequest{appDir, params.modelName, params.croppedImage,
                                    abortFlag.get()});

    if (upscaler->isRequestStale(params.generation) ||
        inferenceResult.error == UpscaylInferenceError::Aborted) {
        QMetaObject::invokeMethod(upscaler, "onTaskAborted", Qt::QueuedConnection,
                                  Q_ARG(QString, params.path), Q_ARG(QSize, params.targetSize), Q_ARG(uint64_t, params.generation));
        return;
    }

    if (!inferenceResult.succeeded()) {
        QString errorMsg;
        if (inferenceResult.error == UpscaylInferenceError::ModelLoadFailed) {
            errorMsg = QCoreApplication::translate("Upscaler", "AI Model failed to load");
        } else {
            errorMsg = QCoreApplication::translate("Upscaler", "AI Upscaling processing failed");
        }
        QMetaObject::invokeMethod(upscaler, "onTaskFailed", Qt::QueuedConnection,
                                  Q_ARG(QString, errorMsg), Q_ARG(uint64_t, params.generation));
        return;
    }

    // The crop keeps the colour space of its source: the viewer colour-manages
    // it on the GPU with the same transform as the image under it.
    QMetaObject::invokeMethod(upscaler, "onTaskFinished", Qt::QueuedConnection,
                              Q_ARG(QImage, inferenceResult.image),
                              Q_ARG(QRect, params.origCrop),
                              Q_ARG(QString, params.path),
                              Q_ARG(QSize, params.targetSize),
                              Q_ARG(uint64_t, params.generation));
}

bool UpscalerRunnable::prepareCrop() {
    if (params.sdrConversion) {
        QImage sdr = HdrToneMapper::toSdr(params.croppedImage, *params.sdrConversion);
        if (sdr.isNull()) {
            qWarning() << "UpscalerRunnable: the HDR crop could not be converted to SDR";
            return false;
        }
        params.croppedImage = std::move(sdr);
    }

    QImage &croppedImg = params.croppedImage;
    QRect &origCrop = params.origCrop;
    if (croppedImg.width() > kMaxCropDimension || croppedImg.height() > kMaxCropDimension) {
        double scaleFactor = (std::min)((double)kMaxCropDimension / origCrop.width(), (double)kMaxCropDimension / origCrop.height());
        croppedImg = croppedImg.scaled(kMaxCropDimension, kMaxCropDimension, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        int w = (croppedImg.width() / 2) * 2;
        int h = (croppedImg.height() / 2) * 2;
        if (w < 2) w = 2;
        if (h < 2) h = 2;
        if (w != croppedImg.width() || h != croppedImg.height()) {
            croppedImg = croppedImg.copy(0, 0, w, h);
        }
        origCrop.setWidth(qRound(w / scaleFactor));
        origCrop.setHeight(qRound(h / scaleFactor));
    }
    return true;
}
