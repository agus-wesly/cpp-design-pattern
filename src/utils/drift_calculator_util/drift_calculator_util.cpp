#include <cmath>
#include "drift_calculator_util.h"
#include "../converter_util/converter_util.h"

double DriftCalculator::calculate_absolute_speed(
    double lat1, double lon1,
    double lat2, double lon2,
    double time_delta)
{

    if (time_delta <= 0.0)
    {
        return 0.0;
    }

    double distance = haversine_distance(lat1, lon1, lat2, lon2);

    return distance / time_delta; // m/s
}

double DriftCalculator::calculateAbsoluteCourse(
    double lat1, double lon1,
    double lat2, double lon2)
{

    // Convert to radians
    double lat1_rad = ConverterUtil::degrees_to_radians(lat1);
    double lon1_rad = ConverterUtil::degrees_to_radians(lon1);
    double lat2_rad = ConverterUtil::degrees_to_radians(lat2);
    double lon2_rad = ConverterUtil::degrees_to_radians(lon2);

    double dlon = lon2_rad - lon1_rad;

    double y = std::sin(dlon) * std::cos(lat2_rad);
    double x = std::cos(lat1_rad) * std::sin(lat2_rad) -
               std::sin(lat1_rad) * std::cos(lat2_rad) * std::cos(dlon);

    double bearing = std::atan2(y, x);

    bearing = std::fmod(bearing + 2 * M_PI, 2 * M_PI);

    return bearing; // radians
}

double DriftCalculator::calculateDriftSpeed(
    double lat1, double lon1,
    double lat2, double lon2,
    double time_delta,
    double relative_speed,
    double heading)
{

    auto absolute_speed = calculate_absolute_speed(lat1, lon1, lat2, lon2, time_delta);
    auto absolute_course = calculateAbsoluteCourse(lat1, lon1, lat2, lon2);

    if (absolute_speed < 0.0 || relative_speed < 0.0)
    {
        return 0.0;
    }

    double angle_diff = absolute_course - heading;

    double drift_speed_squared =
        absolute_speed * absolute_speed +
        relative_speed * relative_speed -
        2.0 * absolute_speed * relative_speed * std::cos(angle_diff);

    if (drift_speed_squared < 0.0)
    {
        drift_speed_squared = 0.0;
    }

    return std::sqrt(drift_speed_squared); // m/s
}

/**
 * Calculate drift course from velocity vectors
 *
 */
double DriftCalculator::calculateDriftCourse(
    double lat1, double lon1,
    double lat2, double lon2,
    double time_delta,
    double relative_speed,
    double heading)
{

    auto absolute_speed = calculate_absolute_speed(lat1, lon1, lat2, lon2, time_delta);
    auto absolute_course = calculateAbsoluteCourse(lat1, lon1, lat2, lon2);

    if (absolute_speed < 0.0 || relative_speed < 0.0)
    {
        return 0.0;
    }

    double absolute_north = absolute_speed * std::cos(absolute_course);
    double absolute_east = absolute_speed * std::sin(absolute_course);

    double relative_north = relative_speed * std::cos(heading);
    double relative_east = relative_speed * std::sin(heading);

    double drift_north = absolute_north - relative_north;
    double drift_east = absolute_east - relative_east;

    double drift_course = std::atan2(drift_east, drift_north);

    drift_course = std::fmod(drift_course + 2 * M_PI, 2 * M_PI);

    return drift_course; // radians
}

double DriftCalculator::haversine_distance(double lat1, double lon1, double lat2, double lon2)
{
    const double R = 6371000.0; // Earth's radius in meters

    // Convert to radians
    double lat1_rad = ConverterUtil::degrees_to_radians(lat1);
    double lon1_rad = ConverterUtil::degrees_to_radians(lon1);
    double lat2_rad = ConverterUtil::degrees_to_radians(lat2);
    double lon2_rad = ConverterUtil::degrees_to_radians(lon2);

    double dlat = lat2_rad - lat1_rad;
    double dlon = lon2_rad - lon1_rad;

    double a = std::sin(dlat / 2.0) * std::sin(dlat / 2.0) +
               std::cos(lat1_rad) * std::cos(lat2_rad) *
                   std::sin(dlon / 2.0) * std::sin(dlon / 2.0);

    double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));

    return R * c; // meters
}