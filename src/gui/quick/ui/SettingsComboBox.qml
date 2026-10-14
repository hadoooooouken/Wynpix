import QtQuick
import QtQuick.Templates as T
import wynpix.style

// Combo box over a settings option list ({value, text} entries of
// SettingsEditorModel): shows the entry of `value` and reports the value
// the person picks. It is as wide as its longest entry, as the widget
// dialog's combo boxes that adjusted to their contents.
ComboBox {
    id: root

    required property var options
    required property var value

    signal picked(var value)

    implicitContentWidthPolicy: T.ComboBox.WidestText
    model: root.options.map(option => option.text)
    currentIndex: root.options.findIndex(option => option.value === root.value)
    onActivated: index => root.picked(root.options[index].value)
}
