#include <QBuffer>
#include <QColor>
#include <QImage>
#include <QImageIOHandler>
#include <QImageIOPlugin>
#include <QPluginLoader>
#include <QTest>

#include <algorithm>
#include <cstdlib>
#include <memory>

#include "testsuites.h"

// APNG playback of the PNG image plugin (3rdparty/kimageformats png.cpp),
// loaded directly so that the other suites keep Qt's own PNG reader. The
// animations are written here, one per way a frame is drawn and disposed
// of, with solid-colour frames whose composed pixels are known.

namespace {
using namespace Qt::StringLiterals;

enum Dispose : quint8 { DisposeNone = 0, DisposeBackground = 1, DisposePrevious = 2 };
enum Blend : quint8 { BlendSource = 0, BlendOver = 1 };
enum ColorType : quint8 { Indexed = 3, Rgba = 6 };

constexpr QSize kCanvas(4, 4);
constexpr QRgb kRed = qRgba(255, 0, 0, 255);
constexpr QRgb kGreen = qRgba(0, 255, 0, 255);
constexpr QRgb kBlue = qRgba(0, 0, 255, 255);
constexpr QRgb kHalfBlue = qRgba(0, 0, 255, 128);
constexpr QRgb kGray = qRgba(128, 128, 128, 255);
constexpr QRgb kClear = qRgba(0, 0, 0, 0);
// Rounding of the premultiplied canvas.
constexpr int kTolerance = 2;

struct FrameSpec {
  QRect rect;
  // RGBA colour, or the palette index of an indexed image.
  QRgb color = kRed;
  quint8 dispose = DisposeNone;
  quint8 blend = BlendSource;
  quint16 delayNum = 1;
  quint16 delayDen = 10;
};

struct ApngSpec {
  quint8 colorType = Rgba;
  QList<QRgb> palette;
  quint32 plays = 0;
  // Writes acTL and fcTL; false writes the first frame as a still PNG.
  bool animated = true;
  // A default image in front of the frames that is not part of the
  // animation.
  bool separateDefaultImage = false;
  QList<FrameSpec> frames;
};

void appendBigEndian32(QByteArray &out, quint32 value) {
  out.append(char(value >> 24));
  out.append(char(value >> 16));
  out.append(char(value >> 8));
  out.append(char(value));
}

void appendBigEndian16(QByteArray &out, quint16 value) {
  out.append(char(value >> 8));
  out.append(char(value));
}

quint32 crc32(QByteArrayView data) {
  quint32 crc = 0xffffffffu;
  for (const char byte : data) {
    crc ^= uchar(byte);
    for (int bit = 0; bit < 8; ++bit)
      crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
  }
  return ~crc;
}

void appendChunk(QByteArray &png, const char *type, const QByteArray &data) {
  appendBigEndian32(png, quint32(data.size()));
  const QByteArray typed = QByteArray(type, 4) + data;
  png.append(typed);
  appendBigEndian32(png, crc32(typed));
}

// A zlib stream of stored (uncompressed) deflate blocks.
QByteArray zlibStored(const QByteArray &raw) {
  QByteArray out("\x78\x01", 2);
  qsizetype pos = 0;
  do {
    const qsizetype size = std::min<qsizetype>(raw.size() - pos, 0xffff);
    out.append(char(pos + size == raw.size() ? 1 : 0));
    out.append(char(size & 0xff));
    out.append(char(size >> 8));
    out.append(char(~size & 0xff));
    out.append(char((~size >> 8) & 0xff));
    out.append(raw.mid(pos, size));
    pos += size;
  } while (pos < raw.size());
  quint32 a = 1;
  quint32 b = 0;
  for (const char byte : raw) {
    a = (a + uchar(byte)) % 65521;
    b = (b + a) % 65521;
  }
  appendBigEndian32(out, (b << 16) | a);
  return out;
}

// The compressed scanlines of a solid size x colour image.
QByteArray solidImageData(QSize size, QRgb color, quint8 colorType) {
  QByteArray pixel;
  if (colorType == Indexed) {
    pixel.append(char(color));
  } else {
    pixel.append(char(qRed(color)));
    pixel.append(char(qGreen(color)));
    pixel.append(char(qBlue(color)));
    pixel.append(char(qAlpha(color)));
  }
  QByteArray raw;
  for (int y = 0; y < size.height(); ++y) {
    raw.append('\0');
    for (int x = 0; x < size.width(); ++x)
      raw.append(pixel);
  }
  return zlibStored(raw);
}

QByteArray makePng(const ApngSpec &spec) {
  QByteArray png("\x89PNG\r\n\x1a\n", 8);
  QByteArray ihdr;
  appendBigEndian32(ihdr, quint32(kCanvas.width()));
  appendBigEndian32(ihdr, quint32(kCanvas.height()));
  ihdr.append(char(8));
  ihdr.append(char(spec.colorType));
  ihdr.append(QByteArray(3, '\0'));
  appendChunk(png, "IHDR", ihdr);
  if (spec.animated) {
    QByteArray actl;
    appendBigEndian32(actl, quint32(spec.frames.size()));
    appendBigEndian32(actl, spec.plays);
    appendChunk(png, "acTL", actl);
  }
  if (!spec.palette.isEmpty()) {
    QByteArray plte;
    QByteArray trns;
    for (const QRgb color : spec.palette) {
      plte.append(char(qRed(color)));
      plte.append(char(qGreen(color)));
      plte.append(char(qBlue(color)));
      trns.append(char(qAlpha(color)));
    }
    appendChunk(png, "PLTE", plte);
    appendChunk(png, "tRNS", trns);
  }

  if (spec.separateDefaultImage)
    appendChunk(png, "IDAT", solidImageData(kCanvas, kGray, spec.colorType));
  quint32 sequence = 0;
  for (qsizetype i = 0; i < spec.frames.size(); ++i) {
    const FrameSpec &frame = spec.frames.at(i);
    if (spec.animated) {
      QByteArray fctl;
      appendBigEndian32(fctl, sequence++);
      appendBigEndian32(fctl, quint32(frame.rect.width()));
      appendBigEndian32(fctl, quint32(frame.rect.height()));
      appendBigEndian32(fctl, quint32(frame.rect.x()));
      appendBigEndian32(fctl, quint32(frame.rect.y()));
      appendBigEndian16(fctl, frame.delayNum);
      appendBigEndian16(fctl, frame.delayDen);
      fctl.append(char(frame.dispose));
      fctl.append(char(frame.blend));
      appendChunk(png, "fcTL", fctl);
    }
    const QByteArray data = solidImageData(frame.rect.size(), frame.color, spec.colorType);
    if (i == 0 && !spec.separateDefaultImage) {
      appendChunk(png, "IDAT", data);
    } else {
      QByteArray fdat;
      appendBigEndian32(fdat, sequence++);
      appendChunk(png, "fdAT", fdat + data);
    }
    if (!spec.animated)
      break;
  }
  appendChunk(png, "IEND", {});
  return png;
}

const QRect kFull(QPoint(0, 0), kCanvas);
const QRect kMiddle(1, 1, 2, 2);

QString describe(QRgb color) {
  return u"(%1, %2, %3, %4)"_s.arg(qRed(color)).arg(qGreen(color)).arg(qBlue(color)).arg(qAlpha(color));
}

// Unpremultiplied pixel colour of a frame.
QRgb pixelAt(const QImage &image, int x, int y) {
  return image.pixelColor(x, y).rgba();
}

bool sameColor(QRgb actual, QRgb expected) {
  // A fully transparent pixel has no colour.
  if (qAlpha(actual) == 0 && qAlpha(expected) == 0)
    return true;
  return std::abs(qRed(actual) - qRed(expected)) <= kTolerance &&
         std::abs(qGreen(actual) - qGreen(expected)) <= kTolerance &&
         std::abs(qBlue(actual) - qBlue(expected)) <= kTolerance &&
         std::abs(qAlpha(actual) - qAlpha(expected)) <= kTolerance;
}

#define COMPARE_PIXEL(image, x, y, expected)                                          \
  QVERIFY2(sameColor(pixelAt(image, x, y), expected),                                 \
           qPrintable(u"pixel (%1, %2) is %3, expected %4"_s.arg(x).arg(y).arg(        \
               describe(pixelAt(image, x, y)), describe(expected))))
} // namespace

class ApngTests : public QObject {
  Q_OBJECT

private:
  QPluginLoader mLoader;
  QImageIOPlugin *mPlugin = nullptr;

  // A handler reading png as format; buffer must outlive it.
  std::unique_ptr<QImageIOHandler> open(QBuffer &buffer, const QByteArray &png,
                                        const QByteArray &format = "apng"_ba) {
    buffer.setData(png);
    buffer.open(QIODevice::ReadOnly);
    return std::unique_ptr<QImageIOHandler>(mPlugin->create(&buffer, format));
  }

  // Every frame of the animation in png.
  QList<QImage> readFrames(const QByteArray &png) {
    QBuffer buffer;
    const auto handler = open(buffer, png);
    QList<QImage> frames;
    while (handler->canRead()) {
      QImage frame;
      if (!handler->read(&frame))
        break;
      frames.append(frame);
    }
    return frames;
  }

private slots:
  void initTestCase() {
#ifdef WYNPIX_TEST_PNG_PLUGIN
    mLoader.setFileName(QStringLiteral(WYNPIX_TEST_PNG_PLUGIN));
    mPlugin = qobject_cast<QImageIOPlugin *>(mLoader.instance());
    QVERIFY2(mPlugin, qPrintable(mLoader.errorString()));
#else
    QSKIP("The PNG image plugin is not built (no libspng).");
#endif
  }

  void sourceBlendReplacesTheArea() {
    const QList<QImage> frames = readFrames(makePng({.frames = {
        {.rect = kFull, .color = kRed},
        {.rect = kMiddle, .color = kHalfBlue, .blend = BlendSource},
    }}));
    QCOMPARE(frames.size(), 2);
    COMPARE_PIXEL(frames[1], 1, 1, kHalfBlue);
    COMPARE_PIXEL(frames[1], 0, 0, kRed);
  }

  void overBlendComposesOntoTheCanvas() {
    const QList<QImage> frames = readFrames(makePng({.frames = {
        {.rect = kFull, .color = kRed},
        {.rect = kMiddle, .color = kHalfBlue, .blend = BlendOver},
    }}));
    QCOMPARE(frames.size(), 2);
    COMPARE_PIXEL(frames[1], 1, 1, qRgba(127, 0, 128, 255));
    COMPARE_PIXEL(frames[1], 3, 3, kRed);
  }

  void backgroundDisposalClearsTheArea() {
    const QList<QImage> frames = readFrames(makePng({.frames = {
        {.rect = kFull, .color = kRed},
        {.rect = kMiddle, .color = kGreen, .dispose = DisposeBackground},
        {.rect = QRect(0, 0, 1, 1), .color = kBlue, .blend = BlendOver},
    }}));
    QCOMPARE(frames.size(), 3);
    COMPARE_PIXEL(frames[1], 1, 1, kGreen);
    COMPARE_PIXEL(frames[2], 1, 1, kClear);
    COMPARE_PIXEL(frames[2], 0, 0, kBlue);
    COMPARE_PIXEL(frames[2], 3, 0, kRed);
  }

  void previousDisposalRestoresTheArea() {
    const QList<QImage> frames = readFrames(makePng({.frames = {
        {.rect = kFull, .color = kRed},
        {.rect = kMiddle, .color = kGreen, .dispose = DisposePrevious},
        {.rect = QRect(3, 3, 1, 1), .color = kBlue},
    }}));
    QCOMPARE(frames.size(), 3);
    COMPARE_PIXEL(frames[1], 1, 1, kGreen);
    COMPARE_PIXEL(frames[2], 1, 1, kRed);
    COMPARE_PIXEL(frames[2], 3, 3, kBlue);
  }

  void previousDisposalOfTheFirstFrameClears() {
    const QList<QImage> frames = readFrames(makePng({.frames = {
        {.rect = kFull, .color = kRed, .dispose = DisposePrevious},
        {.rect = QRect(0, 0, 1, 1), .color = kBlue, .blend = BlendOver},
    }}));
    QCOMPARE(frames.size(), 2);
    COMPARE_PIXEL(frames[0], 1, 1, kRed);
    COMPARE_PIXEL(frames[1], 1, 1, kClear);
    COMPARE_PIXEL(frames[1], 0, 0, kBlue);
  }

  void indexedFramesUseThePaletteTransparency() {
    // Index 0 red, index 1 transparent.
    const QByteArray png = makePng({.colorType = Indexed,
                                    .palette = {kRed, kClear},
                                    .frames = {
                                        {.rect = kFull, .color = 0},
                                        {.rect = kMiddle, .color = 1, .blend = BlendOver},
                                        {.rect = kMiddle, .color = 1, .blend = BlendSource},
                                    }});
    const QList<QImage> frames = readFrames(png);
    QCOMPARE(frames.size(), 3);
    COMPARE_PIXEL(frames[0], 1, 1, kRed);
    COMPARE_PIXEL(frames[1], 1, 1, kRed);
    COMPARE_PIXEL(frames[2], 1, 1, kClear);
    COMPARE_PIXEL(frames[2], 0, 0, kRed);
  }

  void separateDefaultImageIsNotAFrame() {
    const QByteArray png = makePng({.separateDefaultImage = true,
                                    .frames = {
                                        {.rect = kFull, .color = kRed},
                                        {.rect = kFull, .color = kBlue},
                                    }});
    const QList<QImage> frames = readFrames(png);
    QCOMPARE(frames.size(), 2);
    COMPARE_PIXEL(frames[0], 0, 0, kRed);
    COMPARE_PIXEL(frames[1], 0, 0, kBlue);

    // Read as "png", the file is its default image.
    QBuffer buffer;
    const auto handler = open(buffer, png, "png"_ba);
    QImage still;
    QVERIFY(handler->read(&still));
    COMPARE_PIXEL(still, 0, 0, kGray);
  }

  void delaysAndLoopCount_data() {
    QTest::addColumn<quint16>("delayNum");
    QTest::addColumn<quint16>("delayDen");
    QTest::addColumn<int>("delayMs");
    QTest::newRow("50 ms") << quint16(1) << quint16(20) << 50;
    QTest::newRow("denominator 0 means 1/100 s") << quint16(25) << quint16(0) << 250;
    // As in browsers.
    QTest::newRow("no delay plays at 100 ms") << quint16(0) << quint16(100) << 100;
    QTest::newRow("10 ms plays at 100 ms") << quint16(1) << quint16(100) << 100;
  }

  void delaysAndLoopCount() {
    QFETCH(quint16, delayNum);
    QFETCH(quint16, delayDen);
    QFETCH(int, delayMs);
    QBuffer buffer;
    const auto handler = open(buffer, makePng({.plays = 3,
                                               .frames = {
                                                   {.rect = kFull, .delayNum = delayNum, .delayDen = delayDen},
                                                   {.rect = kFull},
                                               }}));
    QVERIFY(handler->supportsOption(QImageIOHandler::Animation));
    QCOMPARE(handler->imageCount(), 2);
    QCOMPARE(handler->loopCount(), 2);
    QImage frame;
    QVERIFY(handler->read(&frame));
    QCOMPARE(handler->nextImageDelay(), delayMs);
  }

  void playingForeverLoopsForever() {
    QBuffer buffer;
    const auto handler = open(buffer, makePng({.plays = 0, .frames = {{.rect = kFull}, {.rect = kFull}}}));
    QCOMPARE(handler->loopCount(), -1);
  }

  void truncatedFilePlaysItsIntactFrames() {
    QByteArray png = makePng({.frames = {
        {.rect = kFull, .color = kRed},
        {.rect = kFull, .color = kGreen},
        {.rect = kFull, .color = kBlue},
    }});
    // Cuts into the last fdAT chunk.
    png.truncate(png.lastIndexOf("fdAT") + 20);
    const QList<QImage> frames = readFrames(png);
    QCOMPARE(frames.size(), 2);
    COMPARE_PIXEL(frames[1], 0, 0, kGreen);
  }

  void readFramesStayUnchanged() {
    const QList<QImage> frames = readFrames(makePng({.frames = {
        {.rect = kFull, .color = kRed},
        {.rect = kMiddle, .color = kGreen},
    }}));
    QCOMPARE(frames.size(), 2);
    COMPARE_PIXEL(frames[0], 1, 1, kRed);
  }

  void jumpingBackComposesTheFrameAgain() {
    const QByteArray png = makePng({.frames = {
        {.rect = kFull, .color = kRed},
        {.rect = kMiddle, .color = kGreen, .dispose = DisposeBackground},
        {.rect = QRect(0, 0, 1, 1), .color = kBlue},
    }});
    QBuffer buffer;
    const auto handler = open(buffer, png);
    QImage frame;
    for (int i = 0; i < 3; ++i)
      QVERIFY(handler->read(&frame));
    QVERIFY(!handler->canRead());

    QVERIFY(handler->jumpToImage(1));
    QVERIFY(handler->read(&frame));
    QCOMPARE(handler->currentImageNumber(), 1);
    COMPARE_PIXEL(frame, 1, 1, kGreen);
    COMPARE_PIXEL(frame, 0, 0, kRed);

    QVERIFY(handler->jumpToImage(0));
    QVERIFY(handler->read(&frame));
    COMPARE_PIXEL(frame, 1, 1, kRed);
  }

  void stillPngReadAsApngIsOneImage() {
    QBuffer buffer;
    const auto handler = open(buffer, makePng({.animated = false, .frames = {{.rect = kFull, .color = kBlue}}}));
    QVERIFY(!handler->supportsOption(QImageIOHandler::Animation));
    QCOMPARE(handler->imageCount(), 1);
    QImage image;
    QVERIFY(handler->read(&image));
    COMPARE_PIXEL(image, 0, 0, kBlue);
  }
};

int runApngTests(int argc, char **argv) {
  ApngTests tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "tst_apng.moc"
