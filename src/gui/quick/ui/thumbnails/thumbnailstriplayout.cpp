#include "thumbnailstriplayout.h"

#include <QSizeF>
#include <QtGlobal>

#include <algorithm>

namespace Enums = SettingsEnums;

namespace {
// Surfaces lighter than this qGray() value get black labels.
constexpr int kLightSurfaceGray = 128;

// The lane of the tiles and the thickness of the panel of the masonry strip.
void applyMasonryGeometry(ThumbnailStripLayout &layout, Enums::PanelPosition position) {
    using Layout = ThumbnailStripLayout;
    layout.laneExtent = layout.horizontal
                            ? qMax(1, qRound(layout.thumbnailSize * Layout::kMasonryLaneAspect))
                            : layout.thumbnailSize;
    // The strip above a bottom panel replaces the border line there.
    const int edge =
        position == Enums::PanelPosition::Bottom ? Layout::kBottomPanelStrip : Layout::kBorderWidth;
    const int tilesExtent = layout.laneExtent + 2 * Layout::kMasonryCrossMargin +
                            Layout::kPanelAllowance +
                            (position == Enums::PanelPosition::Bottom ? Layout::kBottomPanelStrip
                                                                      : 0);
    // The button allowance scales between fixed buttons (at minimum preview
    // size, keeping the panel compact) and full buttons (including page scroll
    // chevrons at larger preview sizes).
    static_assert(Layout::kMasonryFullButtonsSize > Layout::kMasonryMinButtonsSize,
                  "kMasonryFullButtonsSize must exceed kMasonryMinButtonsSize");
    const int fixedExtent =
        Layout::kFixedButtonCount * Layout::kButtonSize + Layout::kButtonSpacing + edge;
    const int fullExtent =
        (Layout::kFixedButtonCount + Layout::kPageButtonCount) * Layout::kButtonSize +
        Layout::kButtonSpacing + edge;
    const qreal t = std::clamp<qreal>(
        static_cast<qreal>(layout.thumbnailSize - Layout::kMasonryMinButtonsSize) /
            (Layout::kMasonryFullButtonsSize - Layout::kMasonryMinButtonsSize),
        qreal{0}, qreal{1});
    const int buttonsExtent = qRound(fixedExtent + t * (fullExtent - fixedExtent));
    layout.panelExtent = qMax(tilesExtent, buttonsExtent);
    // The tiles are centred in the area of the strip.
    layout.laneOffset = (layout.panelExtent - edge - layout.laneExtent) / 2;
}
} // namespace

QColor thumbnailLabelColor(const QColor &background) {
    return qGray(background.rgb()) > kLightSurfaceGray ? QColor(Qt::black) : QColor(Qt::white);
}

QSize ThumbnailStripLayout::imageTargetSize(qreal devicePixelRatio) const {
    return QSize(qRound(devicePixelRatio * imageArea.width()),
                 qRound(devicePixelRatio * imageArea.height()));
}

ThumbnailStripLayout thumbnailStripLayoutFor(const ThumbnailStripLayoutInput &input) {
    ThumbnailStripLayout layout;
    layout.horizontal =
        input.position == Enums::PanelPosition::Top || input.position == Enums::PanelPosition::Bottom;
    layout.thumbnailSize = qBound(ThumbnailStripLayout::kMinimumPreviewSize, input.previewsSize,
                                  ThumbnailStripLayout::kMaximumPreviewSize);
    layout.marginX = layout.horizontal ? ThumbnailStripLayout::kHorizontalMarginX
                                       : ThumbnailStripLayout::kVerticalMarginX;
    layout.marginY = layout.horizontal ? ThumbnailStripLayout::kHorizontalMarginY
                                       : ThumbnailStripLayout::kVerticalMarginY;
    layout.masonry = input.masonry;
    // The masonry strip shows no labels.
    layout.labels = !input.masonry && input.style != SettingsEnums::PanelStyle::Simple;
    layout.textHeight = input.textHeight;

    // ThumbnailWidget::updateBoundingRect(); the strip uses the rounded size
    // (ThumbnailStrip::itemSize()).
    const double imageHeight = layout.thumbnailSize * ThumbnailStripLayout::kImageAspect;
    QSizeF cell(layout.thumbnailSize + (ThumbnailStripLayout::kPadding + layout.marginX) * 2,
                imageHeight + (ThumbnailStripLayout::kPadding + layout.marginY) * 2);
    if (layout.labels)
        cell.rheight() += ThumbnailStripLayout::kLabelSpacing + input.textHeight * 2;
    const QSize cellSize = cell.toSize();
    layout.cellWidth = cellSize.width();
    layout.cellHeight = cellSize.height();

    layout.imageArea = QSize(layout.thumbnailSize, qRound(imageHeight));
    layout.imageCenterOffset = layout.labels ? -input.textHeight : 0;

    // ThumbnailWidget::setupTextLayout(): the rectangle is made of truncated
    // coordinates.
    layout.labelLeft = ThumbnailStripLayout::kPadding + layout.marginX;
    layout.labelTop = static_cast<int>(ThumbnailStripLayout::kPadding + layout.marginY +
                                       imageHeight + ThumbnailStripLayout::kLabelSpacing);
    layout.infoTop = layout.labelTop + input.textHeight + ThumbnailStripLayout::kInfoLineSpacing;
    layout.labelWidth = layout.thumbnailSize;

    // MainPanel::sizeHint().
    if (layout.horizontal) {
        layout.panelExtent = layout.cellHeight + ThumbnailStripLayout::kPanelAllowance;
        if (input.position == Enums::PanelPosition::Bottom)
            layout.panelExtent += ThumbnailStripLayout::kBottomPanelStrip;
    } else {
        layout.panelExtent = layout.cellWidth + ThumbnailStripLayout::kPanelAllowance;
    }
    if (layout.masonry)
        applyMasonryGeometry(layout, input.position);
    return layout;
}

ThumbnailStripLayout thumbnailTileCellFor(QSize size) {
    ThumbnailStripLayout cell;
    cell.horizontal = false;
    cell.thumbnailSize = qMax(size.width(), size.height());
    cell.cellWidth = size.width();
    cell.cellHeight = size.height();
    cell.imageArea = size;
    cell.selectionOverImage = true;
    return cell;
}
