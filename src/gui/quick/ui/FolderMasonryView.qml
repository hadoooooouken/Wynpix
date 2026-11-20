pragma ComponentBehavior: Bound

import QtQuick
import wynpix.bridges
import wynpix.style

// The masonry layout of the folder grid: the folders in a strip of labelled
// cells, and the images below it in rows of equal height that follow their
// proportions, without names or details. FolderGridController decides where
// each tile is; the view draws the tiles near its viewport, which the
// controller's tiles model provides with their positions, and reports its
// viewport. Scrolling is the model's: the view is not interactive.
Flickable {
    id: view

    required property FolderGridController controller
    // Item under the pointer, -1 for none.
    required property int hoveredIndex

    // The view scrolled or changed its height.
    signal viewportChanged

    contentWidth: width
    contentHeight: controller.masonryContentHeight
    interactive: false
    boundsBehavior: Flickable.StopAtBounds

    onContentYChanged: viewportChanged()
    onHeightChanged: viewportChanged()

    Repeater {
        model: view.controller.tiles

        delegate: ThumbnailWidget {
            // The row of the tiles model changes when rows leave in front,
            // the item does not; the cell compares its row with the hovered
            // index.
            required property real tileX
            required property real tileY
            required property int itemIndex

            x: tileX
            y: tileY
            hoveredIndex: view.hoveredIndex === itemIndex ? index : -1
            surfaceColor: Theme.tokens.surfaceView
            hoverColor: Theme.tokens.surfaceViewRaised
            labelOnSurface: view.controller.labelTextColor(Theme.tokens.surfaceView)
            labelOnHover: view.controller.labelTextColor(Theme.tokens.surfaceViewRaised)
            selectedLabelColor: Theme.tokens.textOnView
        }
    }
}
