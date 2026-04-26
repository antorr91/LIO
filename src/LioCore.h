#pragma once

#include <cmath>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace lio {

struct Point2D {
    double x = 0.0;
    double y = 0.0;
};

struct TrackingPoint {
    double time = 0.0;
    double x = 0.0;
    double y = 0.0;
    double likelihood = 1.0;
};

class Trajectory {
public:
    std::vector<TrackingPoint> points;

    [[nodiscard]] bool empty() const noexcept { return points.empty(); }
    [[nodiscard]] std::size_t size() const noexcept { return points.size(); }

    [[nodiscard]] std::optional<TrackingPoint> sampleAtTime(double time) const;
    [[nodiscard]] std::vector<Point2D> pathUntil(double time) const;
};

enum class BehaviorLabel {
    Approach,
    Contact,
    Freezing,
    Grooming,
    Rearing,
    Sniffing,
    SocialInteraction,
    Custom
};

[[nodiscard]] std::string toString(BehaviorLabel label);
[[nodiscard]] BehaviorLabel labelFromHotkey(int key);

enum class AnnotationMode {
    PointInstant,
    Interval
};

struct BehaviorEvent {
    BehaviorLabel label = BehaviorLabel::Approach;
    double startTime = 0.0;
    double endTime = 0.0;
    int startFrame = 0;
    int endFrame = 0;
    Point2D clickPosition {};
    std::string note;

    [[nodiscard]] double duration() const noexcept {
        return endTime - startTime;
    }
};

struct RectROI {
    std::string name = "Target ROI";
    double x = 0.0;
    double y = 0.0;
    double width = 0.0;
    double height = 0.0;
    bool enabled = false;

    [[nodiscard]] bool contains(double px, double py) const noexcept;
    [[nodiscard]] double centerX() const noexcept { return x + width * 0.5; }
    [[nodiscard]] double centerY() const noexcept { return y + height * 0.5; }
};

struct KinematicSummary {
    double totalDistance = 0.0;
    double meanSpeed = 0.0;
    double maxSpeed = 0.0;
};

struct ApproachMetrics {
    bool reachedTarget = false;
    double latencyToFirstEntry = -1.0;
    double totalTimeInTarget = 0.0;
    int numberOfEntries = 0;
    double meanDistanceToTarget = 0.0;
    double minDistanceToTarget = std::numeric_limits<double>::infinity();
};

[[nodiscard]] KinematicSummary computeKinematics(const Trajectory& trajectory);
[[nodiscard]] ApproachMetrics computeApproachMetrics(
    const Trajectory& trajectory,
    const RectROI& target,
    double trialStartTime
);

} // namespace lio
