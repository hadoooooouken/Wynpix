#pragma once

#include <span>

#include "settings_types.h"

// The viewer's scaling filters and its scaling-filter shortcuts
// (toggleScalingFilter, cycleScalingFilter), as decisions independent of any
// UI.
namespace ScalingFilterSelection {

// The filters the viewer offers, in cycle order. The GPU renderer draws all
// of them; the CPU-only filters (QI_FILTER_SMART, QI_FILTER_MKS2021) remain
// for resize, batch conversion and wallpapers only.
[[nodiscard]] std::span<const ScalingFilter> viewingFilters();

// filter as one of viewingFilters(). The CPU-only filters, which 3.6.4 stored
// as the viewing filter, become their GPU counterparts: MKS2021 the same
// kernel, Smart the GPU smart sharpening (a different algorithm, the closest
// one the renderer has). A value outside the enum becomes bilinear.
[[nodiscard]] ScalingFilter forViewing(ScalingFilter filter);

// toggleScalingFilter: switches between the configured filter and nearest
// neighbour; from any filter other than the configured one it returns to the
// configured one.
[[nodiscard]] ScalingFilter toggled(ScalingFilter current, ScalingFilter configured);

// cycleScalingFilter: the next filter of viewingFilters() after
// forViewing(current), wrapping after the last one.
[[nodiscard]] ScalingFilter next(ScalingFilter current);

} // namespace ScalingFilterSelection
