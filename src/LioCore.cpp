#include "LioCore.h"

#include <algorithm>

namespace lio {

std::optional<TrackingPoint> Trajectory::sampleAtTime(double time) const {
    if (points.empty()) {
        return std::nullopt;
    }

    auto it = std::lower_bound(
        points.begin(),
        points.end(),
        time,
        [](const TrackingPoint& p, double t) { return p.time < t; }
    );

    if (it == points.begin()) {
        return *it;
    }

    if (it == points.end()) {
        return points.back();
    }

    const auto& before = *(it - 1);
    const auto& after = *it;

    if (std::abs(before.time - time) <= std::abs(after.time - time)) {
        return before;
    }

    return after;
}

std::vector<Point2D> Trajectory::pathUntil(double time) const {
    std::vector<Point2D> out;
    out.reserve(points.size());

    for (const auto& p : points) {
        if (p.time <= time) {
            out.push_back({p.x, p.y});
        } else {
            break;
        }
    }

    return out;
}

std::string toString(BehaviorLabel label) {
    switch (label) {
        case BehaviorLabel::Approach: return "Approach";
        case BehaviorLabel::Contact: return "Contact";
        case BehaviorLabel::Freezing: return "Freezing";
        case BehaviorLabel::Grooming: return "Grooming";
        case BehaviorLabel::Rearing: return "Rearing";
        case BehaviorLabel::Sniffing: return "Sniffing";
        case BehaviorLabel::SocialInteraction: return "SocialInteraction";
        case BehaviorLabel::Custom: return "Custom";
    }

    return "Custom";
}

BehaviorLabel labelFromHotkey(int key) {
    switch (key) {
        case '1': return BehaviorLabel::Approach;
        case '2': return BehaviorLabel::Contact;
        case '3': return BehaviorLabel::Freezing;
        case '4': return BehaviorLabel::Grooming;
        case '5': return BehaviorLabel::Rearing;
        case '6': return BehaviorLabel::Sniffing;
        case '7': return BehaviorLabel::SocialInteraction;
        default: return BehaviorLabel::Custom;
    }
}

bool RectROI::contains(double px, double py) const noexcept {
    if (!enabled) {
        return false;
    }

    const double x1 = std::min(x, x + width);
    const double x2 = std::max(x, x + width);
    const double y1 = std::min(y, y + height);
    const double y2 = std::max(y, y + height);

    return px >= x1 && px <= x2 && py >= y1 && py <= y2;
}

KinematicSummary computeKinematics(const Trajectory& trajectory) {
    KinematicSummary result;

    if (trajectory.points.size() < 2) {
        return result;
    }

    double totalTime = 0.0;

    for (std::size_t i = 0; i + 1 < trajectory.points.size(); ++i) {
        const auto& p = trajectory.points[i];
        const auto& q = trajectory.points[i + 1];

        const double dt = q.time - p.time;
        if (dt <= 0.0) {
            continue;
        }

        const double dx = q.x - p.x;
        const double dy = q.y - p.y;
        const double distance = std::sqrt(dx * dx + dy * dy);
        const double speed = distance / dt;

        result.totalDistance += distance;
        totalTime += dt;
        result.maxSpeed = std::max(result.maxSpeed, speed);
    }

    if (totalTime > 0.0) {
        result.meanSpeed = result.totalDistance / totalTime;
    }

    return result;
}

ApproachMetrics computeApproachMetrics(
    const Trajectory& trajectory,
    const RectROI& target,
    double trialStartTime
) {
    ApproachMetrics result;

    if (!target.enabled || trajectory.points.size() < 2) {
        return result;
    }

    bool wasInside = false;
    double distanceSum = 0.0;
    int distanceCount = 0;

    for (std::size_t i = 0; i + 1 < trajectory.points.size(); ++i) {
        const auto& p = trajectory.points[i];
        const auto& q = trajectory.points[i + 1];

        const double dt = q.time - p.time;
        if (dt <= 0.0) {
            continue;
        }

        const bool isInside = target.contains(p.x, p.y);

        if (isInside) {
            result.totalTimeInTarget += dt;
        }

        if (isInside && !wasInside) {
            result.numberOfEntries++;

            if (!result.reachedTarget) {
                result.reachedTarget = true;
                result.latencyToFirstEntry = p.time - trialStartTime;
            }
        }

        wasInside = isInside;

        const double dx = p.x - target.centerX();
        const double dy = p.y - target.centerY();
        const double distance = std::sqrt(dx * dx + dy * dy);

        distanceSum += distance;
        distanceCount++;
        result.minDistanceToTarget = std::min(result.minDistanceToTarget, distance);
    }

    if (distanceCount > 0) {
        result.meanDistanceToTarget = distanceSum / distanceCount;
    }

    return result;
}

} // namespace lio
