#include <QColorSpace>
#include <QImage>
#include <QTest>

#include "testsuites.h"
#include "utils/hdrsource.h"

// HDR detection shared by the CPU tone mapper and the GPU renderer
// (utils/hdrsource.h). The render tests compare the two, so they cannot
// notice a case both decode the same wrong way; the expected encodings here
// are written out from the image plugins' tags (3rdparty/kimageformats
// avif.cpp and jxl.cpp: HDR_Transfer "PQ" / "HLG", HDR_Primaries "BT.2020" /
// "Display P3" / "sRGB") and from the colour space alone.

Q_DECLARE_METATYPE(HdrTransfer)
Q_DECLARE_METATYPE(HdrPrimaries)

namespace {
using namespace Qt::StringLiterals;

constexpr QSize kImageSize(4, 4);

struct Tags {
  QString transfer;
  QString primaries;
};

// An 8-bit image as the AVIF and JXL plugins return an HDR image: PQ or HLG
// samples described by the HDR_* text keys.
QImage taggedImage(const Tags &tags) {
  QImage image(kImageSize, QImage::Format_RGBX64);
  image.fill(Qt::gray);
  image.setText(u"HDR_IsHDR"_s, u"true"_s);
  if (!tags.transfer.isEmpty())
    image.setText(u"HDR_Transfer"_s, tags.transfer);
  if (!tags.primaries.isEmpty())
    image.setText(u"HDR_Primaries"_s, tags.primaries);
  return image;
}

QImage imageIn(QImage::Format format, const QColorSpace &space) {
  QImage image(kImageSize, format);
  image.fill(Qt::gray);
  if (space.isValid())
    image.setColorSpace(space);
  return image;
}
} // namespace

class HdrSourceTests : public QObject {
  Q_OBJECT

private slots:
  void hdrImagesAreRecognised_data() {
    QTest::addColumn<QImage>("image");
    QTest::addColumn<bool>("hdr");

    const QColorSpace srgb(QColorSpace::SRgb);
    const QColorSpace pq(QColorSpace::Primaries::Bt2020, QColorSpace::TransferFunction::St2084);
    const QColorSpace hlg(QColorSpace::Primaries::Bt2020, QColorSpace::TransferFunction::Hlg);
    QColorSpace described(QColorSpace::SRgb);
    described.setDescription(u"Rec. 2100 PQ"_s);

    QTest::newRow("null image") << QImage() << false;
    QTest::newRow("8-bit sRGB") << imageIn(QImage::Format_RGB32, srgb) << false;
    QTest::newRow("8-bit untagged") << imageIn(QImage::Format_RGB32, {}) << false;
    QTest::newRow("16-bit integer sRGB") << imageIn(QImage::Format_RGBX64, srgb) << false;
    QTest::newRow("HDR_IsHDR tag") << taggedImage({}) << true;
    QImage profileOnly = imageIn(QImage::Format_RGB32, {});
    profileOnly.setText(u"HDR_Profile"_s, u"BT.2100 HLG"_s);
    QTest::newRow("HDR_Profile tag") << profileOnly << true;
    QImage notHdr = imageIn(QImage::Format_RGB32, {});
    notHdr.setText(u"HDR_IsHDR"_s, u"false"_s);
    QTest::newRow("HDR_IsHDR false") << notHdr << false;
    QTest::newRow("half float") << imageIn(QImage::Format_RGBA16FPx4, {}) << true;
    QTest::newRow("float") << imageIn(QImage::Format_RGBX32FPx4, {}) << true;
    QTest::newRow("PQ colour space") << imageIn(QImage::Format_RGBX64, pq) << true;
    QTest::newRow("HLG colour space") << imageIn(QImage::Format_RGBX64, hlg) << true;
    QTest::newRow("HDR description") << imageIn(QImage::Format_RGBX64, described) << true;
  }

  void hdrImagesAreRecognised() {
    QFETCH(QImage, image);
    QFETCH(bool, hdr);
    QCOMPARE(isHdrImage(image), hdr);
  }

  void encodingFollowsTheTagsAndTheColourSpace_data() {
    QTest::addColumn<QImage>("image");
    QTest::addColumn<HdrTransfer>("transfer");
    QTest::addColumn<HdrPrimaries>("primaries");

    // Every combination the AVIF and JXL plugins tag.
    QTest::newRow("PQ BT.2020") << taggedImage({u"PQ"_s, u"BT.2020"_s}) << HdrTransfer::PQ
                                << HdrPrimaries::Bt2020;
    QTest::newRow("PQ Display P3") << taggedImage({u"PQ"_s, u"Display P3"_s})
                                   << HdrTransfer::PQ << HdrPrimaries::DisplayP3;
    QTest::newRow("PQ sRGB") << taggedImage({u"PQ"_s, u"sRGB"_s}) << HdrTransfer::PQ
                             << HdrPrimaries::Srgb;
    QTest::newRow("HLG BT.2020") << taggedImage({u"HLG"_s, u"BT.2020"_s}) << HdrTransfer::HLG
                                 << HdrPrimaries::Bt2020;
    QTest::newRow("HLG Display P3") << taggedImage({u"HLG"_s, u"Display P3"_s})
                                    << HdrTransfer::HLG << HdrPrimaries::DisplayP3;
    QTest::newRow("HLG sRGB") << taggedImage({u"HLG"_s, u"sRGB"_s}) << HdrTransfer::HLG
                              << HdrPrimaries::Srgb;

    // Other spellings of the same tags.
    QTest::newRow("lower-case hlg") << taggedImage({u"hlg"_s, {}}) << HdrTransfer::HLG
                                    << HdrPrimaries::Bt2020;
    QTest::newRow("Linear tag") << taggedImage({u"Linear"_s, {}}) << HdrTransfer::Linear
                                << HdrPrimaries::Bt2020;
    QTest::newRow("BT.709 tag") << taggedImage({u"PQ"_s, u"BT.709"_s}) << HdrTransfer::PQ
                                << HdrPrimaries::Srgb;
    QTest::newRow("untagged transfer and primaries") << taggedImage({}) << HdrTransfer::PQ
                                                     << HdrPrimaries::Bt2020;

    // The colour space alone.
    QTest::newRow("PQ BT.2020 space")
        << imageIn(QImage::Format_RGBX64, QColorSpace(QColorSpace::Primaries::Bt2020,
                                                      QColorSpace::TransferFunction::St2084))
        << HdrTransfer::PQ << HdrPrimaries::Bt2020;
    QTest::newRow("HLG BT.2020 space")
        << imageIn(QImage::Format_RGBX64, QColorSpace(QColorSpace::Primaries::Bt2020,
                                                      QColorSpace::TransferFunction::Hlg))
        << HdrTransfer::HLG << HdrPrimaries::Bt2020;
    QTest::newRow("PQ Display P3 space")
        << imageIn(QImage::Format_RGBX64, QColorSpace(QColorSpace::Primaries::DciP3D65,
                                                      QColorSpace::TransferFunction::St2084))
        << HdrTransfer::PQ << HdrPrimaries::DisplayP3;
    QTest::newRow("linear sRGB space (scRGB)")
        << imageIn(QImage::Format_RGBA16FPx4, QColorSpace(QColorSpace::SRgbLinear))
        << HdrTransfer::Linear << HdrPrimaries::Srgb;
    QTest::newRow("linear BT.2020 space")
        << imageIn(QImage::Format_RGBA32FPx4, QColorSpace(QColorSpace::Primaries::Bt2020,
                                                          QColorSpace::TransferFunction::Linear))
        << HdrTransfer::Linear << HdrPrimaries::Bt2020;
    QTest::newRow("untagged half float")
        << imageIn(QImage::Format_RGBA16FPx4, {}) << HdrTransfer::Linear << HdrPrimaries::Bt2020;
  }

  void encodingFollowsTheTagsAndTheColourSpace() {
    QFETCH(QImage, image);
    QFETCH(HdrTransfer, transfer);
    QFETCH(HdrPrimaries, primaries);
    const HdrSourceEncoding encoding = detectHdrSourceEncoding(image);
    QCOMPARE(encoding.transfer, transfer);
    QCOMPARE(encoding.primaries, primaries);
  }
};

int runHdrSourceTests(int argc, char **argv) {
  HdrSourceTests tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "tst_hdrsource.moc"
