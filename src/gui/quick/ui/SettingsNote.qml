import QtQuick
import QtQuick.Layouts
import wynpix.bridges
import wynpix.style

// Explanation under an option of the settings window, in the label text
// colour at half opacity (the SNoteText labels of the widget dialog).
Label {
    id: root

    readonly property real noteTextOpacity: 0.5

    Layout.fillWidth: true
    color: Color.transparent(Theme.colors.textHc, root.noteTextOpacity)
    font: Theme.fonts.caption
    textFormat: Text.PlainText
    wrapMode: Text.WordWrap
}
