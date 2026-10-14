#include "scalingfilterselection.h"

#include <algorithm>
#include <array>
#include <iterator>

namespace {
constexpr std::array kViewingFilters{
    QI_FILTER_NEAREST,   QI_FILTER_BILINEAR,    QI_FILTER_CAS,
    QI_FILTER_SMART_GPU, QI_FILTER_MKS2021_GPU,
};

// Same fallback as Settings::scalingFilter() for an unknown stored value.
constexpr ScalingFilter kUnknownFilterFallback = QI_FILTER_BILINEAR;
} // namespace

std::span<const ScalingFilter> ScalingFilterSelection::viewingFilters() {
    return kViewingFilters;
}

ScalingFilter ScalingFilterSelection::forViewing(ScalingFilter filter) {
    switch (filter) {
    case QI_FILTER_SMART:
        return QI_FILTER_SMART_GPU;
    case QI_FILTER_MKS2021:
        return QI_FILTER_MKS2021_GPU;
    case QI_FILTER_NEAREST:
    case QI_FILTER_BILINEAR:
    case QI_FILTER_CAS:
    case QI_FILTER_SMART_GPU:
    case QI_FILTER_MKS2021_GPU:
        return filter;
    }
    return kUnknownFilterFallback;
}

ScalingFilter ScalingFilterSelection::toggled(ScalingFilter current, ScalingFilter configured) {
    return current == configured ? QI_FILTER_NEAREST : configured;
}

ScalingFilter ScalingFilterSelection::next(ScalingFilter current) {
    const auto position = std::ranges::find(kViewingFilters, forViewing(current));
    if (position == kViewingFilters.end() || std::next(position) == kViewingFilters.end())
        return kViewingFilters.front();
    return *std::next(position);
}
