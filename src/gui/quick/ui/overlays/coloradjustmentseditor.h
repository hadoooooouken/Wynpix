#pragma once

#include <QtQml/qqmlregistration.h>

#include "gui/ports/notificationport.h"
#include "gui/quick/ui/overlays/adjustmentslidermodel.h"
#include "utils/coloradjustments.h"

// Sliders of the colour adjustments overlay (ColorAdjustmentsOverlay.qml),
// with the ranges, defaults and value texts of the widget overlay. Every edit
// previews the adjustments live (previewChanged(); the GPU renderer applies
// them per frame, so no throttling is needed). Compare previews the
// unadjusted image while held; apply requests the edit and resets the
// sliders; copy notifies that the adjustments were copied for the batch
// converter.
//
// Owned by OverlayCoordinator. GUI thread only.
class ColorAdjustmentsEditor final : public AdjustmentSliderModel {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by OverlayCoordinator")
    Q_PROPERTY(bool hasAdjustments READ hasAdjustments NOTIFY hasAdjustmentsChanged FINAL)

public:
    // Row order of the overlay; the batch converter uses the same order.
    enum Row { SmartEnhance, Exposure, Contrast, Brightness, Temperature, Tint, Hue, Vibrance, Saturation, RowCount };
    Q_ENUM(Row)

    explicit ColorAdjustmentsEditor(QObject *parent = nullptr);

    [[nodiscard]] ColorAdjustments adjustments() const;
    [[nodiscard]] bool hasAdjustments() const;

    // Shows the image without adjustments while comparing is true.
    Q_INVOKABLE void setComparing(bool comparing);
    // Requests the adjustments to be applied to the image and resets the
    // sliders.
    Q_INVOKABLE void apply();
    // Copies the adjustments for the batch converter and requests a
    // confirmation notification.
    Q_INVOKABLE void copy();

signals:
    // The adjustments the viewer shows now.
    void previewChanged(const ColorAdjustments &adjustments);
    void applyRequested(const ColorAdjustments &adjustments);
    void notificationRequested(const NotificationRequest &request);
    void colorAdjustmentsCopied(const ColorAdjustments &adjustments);
    void hasAdjustmentsChanged();

private:
    void valuesEdited() override;

    bool mComparing = false;
    bool mHasAdjustments = false;
};
