/*
    PNG image format plugin for Wynpix, decoding via libspng.
*/

#ifndef KIMG_PNG_P_H
#define KIMG_PNG_P_H

#include <QImageIOPlugin>
#include <QScopedPointer>

class SpngHandlerPrivate;
class SpngHandler : public QImageIOHandler
{
public:
    SpngHandler();

    bool canRead() const override;
    bool read(QImage *image) override;

    bool supportsOption(QImageIOHandler::ImageOption option) const override;
    QVariant option(QImageIOHandler::ImageOption option) const override;

    // Animation of an APNG, used when the handler is created for the "apng"
    // format. Read as "png", an APNG gives its default image.
    int imageCount() const override;
    int loopCount() const override;
    int nextImageDelay() const override;
    int currentImageNumber() const override;
    bool jumpToNextImage() override;
    bool jumpToImage(int imageNumber) override;

    static bool canRead(QIODevice *device);

private:
    bool isAnimation() const;
    bool composeNextFrame();

    const QScopedPointer<SpngHandlerPrivate> d;
};

class SpngPlugin : public QImageIOPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QImageIOHandlerFactoryInterface" FILE "png.json")

public:
    Capabilities capabilities(QIODevice *device, const QByteArray &format) const override;
    QImageIOHandler *create(QIODevice *device, const QByteArray &format = QByteArray()) const override;
};

#endif // KIMG_PNG_P_H
