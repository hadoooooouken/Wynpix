import QtQuick
import wynpix.bridges
import wynpix.style

// Colour adjustment sliders with a live preview (ColorAdjustmentsOverlay in
// the widget UI). It opens at the pointer and can be dragged by its header.
// Compare shows the unadjusted image while held; Apply applies the
// adjustments to the image; Reset restores the defaults.
Item {
    id: root

    required property OverlayCoordinator coordinator
    readonly property ColorAdjustmentsEditor editor: coordinator.colorAdjustmentsEditor

    readonly property int panelWidth: 420
    readonly property int padding: 12
    readonly property int contentSpacing: 8
    readonly property int buttonSpacing: 6

    visible: coordinator.colorAdjustments.open

    Connections {
        target: root.coordinator.colorAdjustments
        function onOpenChanged() {
            if (root.coordinator.colorAdjustments.open)
                panel.resetPosition();
        }
    }

    Gradient {
        id: smartEnhanceGradient
        orientation: Gradient.Horizontal
        GradientStop { position: 0.0; color: "#0B2B47" }
        GradientStop { position: 0.48; color: "#0284C7" }
        GradientStop { position: 1.0; color: "#D8E6C8" }
    }

    // Blend gradient for Exposure, Contrast and Brightness: layers dark and
    // light shading over the active Theme.colors.accent base.
    Gradient {
        id: toneBlendGradient
        orientation: Gradient.Horizontal
        GradientStop { position: 0.0; color: Qt.rgba(0, 0, 0, 0.85) }
        GradientStop { position: 0.5; color: Qt.rgba(0, 0, 0, 0.0) }
        GradientStop { position: 1.0; color: Qt.rgba(1, 1, 1, 0.45) }
    }

    Gradient {
        id: temperatureGradient
        orientation: Gradient.Horizontal
        GradientStop { position: 0.0; color: "#2563EB" }
        GradientStop { position: 0.5; color: "#CBD5E1" }
        GradientStop { position: 1.0; color: "#F59E0B" }
    }

    Gradient {
        id: tintGradient
        orientation: Gradient.Horizontal
        GradientStop { position: 0.0; color: "#16A34A" }
        GradientStop { position: 0.5; color: "#CBD5E1" }
        GradientStop { position: 1.0; color: "#C026D3" }
    }

    Gradient {
        id: hueGradient
        orientation: Gradient.Horizontal
        GradientStop { position: 0.0; color: "#EF4444" }
        GradientStop { position: 0.17; color: "#EAB308" }
        GradientStop { position: 0.33; color: "#22C55E" }
        GradientStop { position: 0.50; color: "#06B6D4" }
        GradientStop { position: 0.67; color: "#3B82F6" }
        GradientStop { position: 0.83; color: "#A855F7" }
        GradientStop { position: 1.0; color: "#EF4444" }
    }

    Gradient {
        id: vibranceGradient
        orientation: Gradient.Horizontal
        GradientStop { position: 0.0; color: "#64748B" }
        GradientStop { position: 0.5; color: "#C2A27A" }
        GradientStop { position: 1.0; color: "#F97316" }
    }

    Gradient {
        id: saturationGradient
        orientation: Gradient.Horizontal
        GradientStop { position: 0.0; color: "#64748B" }
        GradientStop { position: 0.55; color: "#EAB308" }
        GradientStop { position: 1.0; color: "#EF4444" }
    }

    readonly property var sliderTracks: [
        { gradient: smartEnhanceGradient },
        { gradient: toneBlendGradient, baseColor: Theme.colors.accent },
        { gradient: toneBlendGradient, baseColor: Theme.colors.accent },
        { gradient: toneBlendGradient, baseColor: Theme.colors.accent },
        { gradient: temperatureGradient },
        { gradient: tintGradient },
        { gradient: hueGradient },
        { gradient: vibranceGradient },
        { gradient: saturationGradient }
    ]

    OverlayPanel {
        id: panel

        icon: FluentIcons.Adjustments20
        title: qsTr("Color adjustments")
        strongTitle: true
        draggable: true
        width: root.panelWidth
        bottomPadding: root.padding
        placementX: Math.max(0, Math.min(root.coordinator.colorAdjustments.anchor.x, root.width - width))
        placementY: Math.max(0, Math.min(root.coordinator.colorAdjustments.anchor.y, root.height - height))
        onCloseClicked: root.coordinator.colorAdjustments.close()

        Column {
            x: root.padding
            topPadding: root.contentSpacing
            width: parent.width - 2 * root.padding
            spacing: root.contentSpacing

            AdjustmentSliders {
                width: parent.width
                sliders: root.editor
                trackGradients: root.sliderTracks
            }

            Row {
                spacing: root.buttonSpacing

                Button {
                    text: qsTr("Compare")
                    focusPolicy: Qt.NoFocus
                    onPressed: root.editor.setComparing(true)
                    onReleased: root.editor.setComparing(false)
                    onCanceled: root.editor.setComparing(false)
                }
                Button {
                    text: qsTr("Apply")
                    focusPolicy: Qt.NoFocus
                    onClicked: root.editor.apply()
                }
                Button {
                    text: qsTr("Reset")
                    focusPolicy: Qt.NoFocus
                    onClicked: root.editor.resetAll()
                }
            }
        }
    }
}
