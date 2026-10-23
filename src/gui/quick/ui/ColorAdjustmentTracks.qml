import QtQuick
import wynpix.bridges

// Shared gradient specifications for ColorAdjustmentsOverlay and
// BatchConverterDialog color sliders.
Item {
    id: root

    visible: false

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

    readonly property var tracks: [
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
}
