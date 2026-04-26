#pragma once

#include "LioCore.h"

#include <filesystem>
#include <string>
#include <vector>

namespace lio {

[[nodiscard]] Trajectory loadTrajectoryCsv(const std::filesystem::path& path);
void saveAnnotationsCsv(const std::filesystem::path& path, const std::vector<BehaviorEvent>& events);
void saveSummaryCsv(
    const std::filesystem::path& path,
    const KinematicSummary& kinematics,
    const ApproachMetrics& approach
);

} // namespace lio
