#pragma once

#include <QColor>
#include <QObject>
#include <QSize>
#include <QtQml/qqmlregistration.h>

#include "components/settingseditor/settingsscales.h"
#include "gui/quick/bridges/settingsenums.h"

// What the size of the thumbnail strip depends on.
struct ThumbnailStripLayoutInput {
    // The panelPreviewsSize setting (bounded to kMinimumPreviewSize -
    // kMaximumPreviewSize).
    int previewsSize = 0;
    SettingsEnums::PanelStyle style = SettingsEnums::PanelStyle::Simple;
    SettingsEnums::PanelPosition position = SettingsEnums::PanelPosition::Top;
    // Line height of the label font (QFontMetrics::height()).
    int textHeight = 0;
    // The strip lays its tiles out by the proportions of the images
    // (StripMasonryLayout) and shows no labels.
    bool masonry = false;
};

// Geometry of the thumbnail strip and its cells, as the widget ThumbnailStrip
// and ThumbnailWidget compute it, in logical pixels. A cell is the thumbnail
// area (thumbnailSize x 3/4 of it) inside padding and margins, plus two label
// lines for the extended style; the highlight and hover surfaces cover the
// cell minus its margins. In the masonry strip the tiles are as high (wide in
// a vertical strip) as the lane, so that a 16:9 image is as wide as the
// preview size; the panel is as thick as the tiles and the scroll bar need,
// but never thinner than its button allowance needs (which scales from fixed
// buttons at minimum preview size to full buttons at larger sizes). QML lays
// the strip and the cells out from these values only.
struct ThumbnailStripLayout {
    Q_GADGET
    QML_VALUE_TYPE(thumbnailStripLayout)
    QML_UNCREATABLE("Provided by ThumbnailPanelController.layout")
    Q_PROPERTY(bool horizontal MEMBER horizontal FINAL)
    Q_PROPERTY(int thumbnailSize MEMBER thumbnailSize FINAL)
    Q_PROPERTY(int cellWidth MEMBER cellWidth FINAL)
    Q_PROPERTY(int cellHeight MEMBER cellHeight FINAL)
    Q_PROPERTY(int itemExtent READ itemExtent FINAL)
    Q_PROPERTY(int marginX MEMBER marginX FINAL)
    Q_PROPERTY(int marginY MEMBER marginY FINAL)
    Q_PROPERTY(QSize imageArea MEMBER imageArea FINAL)
    Q_PROPERTY(int imageCenterOffset MEMBER imageCenterOffset FINAL)
    Q_PROPERTY(bool labels MEMBER labels FINAL)
    Q_PROPERTY(int labelLeft MEMBER labelLeft FINAL)
    Q_PROPERTY(int labelTop MEMBER labelTop FINAL)
    Q_PROPERTY(int infoTop MEMBER infoTop FINAL)
    Q_PROPERTY(int labelWidth MEMBER labelWidth FINAL)
    Q_PROPERTY(int textHeight MEMBER textHeight FINAL)
    Q_PROPERTY(int panelExtent MEMBER panelExtent FINAL)
    Q_PROPERTY(bool selectionOverImage MEMBER selectionOverImage FINAL)
    Q_PROPERTY(bool masonry MEMBER masonry FINAL)
    Q_PROPERTY(int buttonSize READ buttonSize CONSTANT FINAL)
    Q_PROPERTY(int buttonSpacing READ buttonSpacing CONSTANT FINAL)
    Q_PROPERTY(int borderWidth READ borderWidth CONSTANT FINAL)
    Q_PROPERTY(int fixedButtonCount READ fixedButtonCount CONSTANT FINAL)
    Q_PROPERTY(int pageButtonCount READ pageButtonCount CONSTANT FINAL)

public:
    static constexpr int kMinimumPreviewSize = 20;
    static constexpr int kMaximumPreviewSize = SettingsScales::kMaxPanelSizePixels;
    // Padding between the cell surface and the thumbnail area.
    static constexpr int kPadding = 9;
    // Cell margins of a horizontal strip (top / bottom panel) and a vertical
    // one (left / right panel).
    static constexpr int kHorizontalMarginX = 2;
    static constexpr int kHorizontalMarginY = 4;
    static constexpr int kVerticalMarginX = 12;
    static constexpr int kVerticalMarginY = 2;
    // Space between the thumbnail area and the name label, and between the
    // name and the info label.
    static constexpr int kLabelSpacing = 9;
    static constexpr int kInfoLineSpacing = 2;
    // Height of the thumbnail area relative to its width.
    static constexpr double kImageAspect = 0.75;
    // Panel thickness beyond the cell: scroll bar and border, plus the strip
    // above a bottom panel.
    static constexpr int kPanelAllowance = 16;
    static constexpr int kBottomPanelStrip = 3;
    // Height of a tile of a horizontal masonry strip relative to the preview
    // size, so that a 16:9 image is as wide as the preview size. The tiles of
    // a vertical strip are as wide as the preview size.
    static constexpr double kMasonryLaneAspect = 9.0 / 16.0;
    // Space between the tiles of a masonry strip and the panel edges across
    // the strip.
    static constexpr int kMasonryCrossMargin = 4;
    // The buttons of the panel (MainPanel.qml takes them from the layout):
    // the edge of a button, the space between buttons and around them, the
    // border line of the panel, the buttons that are always there (settings,
    // folder view, pin, the strip mode) and the page buttons.
    static constexpr int kButtonSize = 30;
    static constexpr int kButtonSpacing = 4;
    static constexpr int kBorderWidth = 1;
    static constexpr int kFixedButtonCount = 4;
    static constexpr int kPageButtonCount = 2;
    // Preview sizes where the masonry panel button allowance scales between
    // fixed buttons (compact panel at minimum preview size) and full buttons
    // (room for page scroll buttons at larger preview sizes).
    static constexpr int kMasonryMinButtonsSize = SettingsScales::kMinPanelSizePixels;
    static constexpr int kMasonryFullButtonsSize = 200;
    static_assert(kMasonryFullButtonsSize > kMasonryMinButtonsSize,
                  "Masonry button scaling full size must exceed min size");

    // The strip runs from left to right (top and bottom panel).
    bool horizontal = true;
    int thumbnailSize = 0;
    int cellWidth = 0;
    int cellHeight = 0;
    int marginX = 0;
    int marginY = 0;
    // Area the thumbnail is fitted into (thumbnailDrawSize()).
    QSize imageArea;
    // Vertical offset of the thumbnail centre from the cell centre.
    int imageCenterOffset = 0;
    // The extended style shows the name and the info line under the image.
    bool labels = false;
    int labelLeft = 0;
    int labelTop = 0;
    int infoTop = 0;
    int labelWidth = 0;
    int textHeight = 0;
    // Thickness of the panel across the strip.
    int panelExtent = 0;
    // The thumbnail fills the cell (a tile of the masonry view), so the
    // selection is drawn over it instead of around it.
    bool selectionOverImage = false;
    // The strip is the masonry strip: laneExtent is the size of its tiles
    // across it, laneOffset the distance from the start of the strip, across
    // it, to the tiles.
    bool masonry = false;
    int laneExtent = 0;
    int laneOffset = 0;

    // Size of a cell along the strip.
    [[nodiscard]] int itemExtent() const { return horizontal ? cellWidth : cellHeight; }

    // The buttons of the panel, for MainPanel.qml.
    [[nodiscard]] int buttonSize() const { return kButtonSize; }
    [[nodiscard]] int buttonSpacing() const { return kButtonSpacing; }
    [[nodiscard]] int borderWidth() const { return kBorderWidth; }
    [[nodiscard]] int fixedButtonCount() const { return kFixedButtonCount; }
    [[nodiscard]] int pageButtonCount() const { return kPageButtonCount; }

    // The box, in device pixels, thumbnails are made for: the image area
    // times the device pixel ratio. A thumbnail of this size is drawn into
    // the area without a resize.
    [[nodiscard]] QSize imageTargetSize(qreal devicePixelRatio) const;

    friend bool operator==(const ThumbnailStripLayout &, const ThumbnailStripLayout &) = default;
};

[[nodiscard]] ThumbnailStripLayout thumbnailStripLayoutFor(const ThumbnailStripLayoutInput &input);

// The cell of a tile of the masonry views: the thumbnail fills it, without
// padding, margins or labels.
[[nodiscard]] ThumbnailStripLayout thumbnailTileCellFor(QSize size);

// Text colour of an unselected cell label on background: black on light
// surfaces, white on dark ones (ThumbnailWidget::drawLabel()).
[[nodiscard]] QColor thumbnailLabelColor(const QColor &background);
