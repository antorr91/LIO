#include "CsvIO.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace lio {

static std::vector<std::string> splitComma(const std::string& line) {
    std::vector<std::string> values;
    std::stringstream ss(line);
    std::string item;

    while (std::getline(ss, item, ',')) {
        values.push_back(item);
    }

    return values;
}

Trajectory loadTrajectoryCsv(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Could not open tracking CSV: " + path.string());
    }

    Trajectory trajectory;
    std::string line;

    // Header
    if (!std::getline(in, line)) {
        return trajectory;
    }

    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }

        const auto values = splitComma(line);
        if (values.size() < 3) {
            continue;
        }

        TrackingPoint point;
        point.time = std::stod(values[0]);
        point.x = std::stod(values[1]);
        point.y = std::stod(values[2]);

        if (values.size() >= 4) {
            point.likelihood = std::stod(values[3]);
        }

        trajectory.points.push_back(point);
    }

    return trajectory;
}

void saveAnnotationsCsv(const std::filesystem::path& path, const std::vector<BehaviorEvent>& events) {
    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("Could not write annotations CSV: " + path.string());
    }

    out << "label,start_time,end_time,duration,start_frame,end_frame,x,y,note\n";

    for (const auto& e : events) {
        out << toString(e.label) << ','
            << e.startTime << ','
            << e.endTime << ','
            << e.duration() << ','
            << e.startFrame << ','
            << e.endFrame << ','
            << e.clickPosition.x << ','
            << e.clickPosition.y << ','
            << '"' << e.note << '"' << '\n';
    }
}

void saveSummaryCsv(
    const std::filesystem::path& path,
    const KinematicSummary& kinematics,
    const ApproachMetrics& approach
) {
    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("Could not write summary CSV: " + path.string());
    }

    out << "metric,value\n";
    out << "total_distance_px," << kinematics.totalDistance << "\n";
    out << "mean_speed_px_per_s," << kinematics.meanSpeed << "\n";
    out << "max_speed_px_per_s," << kinematics.maxSpeed << "\n";
    out << "reached_target," << (approach.reachedTarget ? 1 : 0) << "\n";
    out << "latency_to_first_entry_s," << approach.latencyToFirstEntry << "\n";
    out << "total_time_in_target_s," << approach.totalTimeInTarget << "\n";
    out << "number_of_entries," << approach.numberOfEntries << "\n";
    out << "mean_distance_to_target_px," << approach.meanDistanceToTarget << "\n";
    out << "min_distance_to_target_px," << approach.minDistanceToTarget << "\n";
}

} // namespace lio
