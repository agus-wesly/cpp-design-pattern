#pragma once

class DriftCalculator {
    public:
        /**
         * Calculate absolute speed (speed over ground) from position changes
         */
        static double calculate_absolute_speed(
                double lat1, double lon1,
                double lat2, double lon2,
                double time_delta);

        /**
         * Calculate absolute course (course over ground) from position changes
         * 
         */   
        static double calculateAbsoluteCourse(
                double lat1, double lon1,
                double lat2, double lon2);

        /**
         * Calculate drift speed from absolute and relative speeds
         * 
         */
        static double calculateDriftSpeed(
                double lat1, double lon1,
                double lat2, double lon2,
                double time_delta,
                double relative_speed,
                double heading);

        /**
         * Calculate drift course from velocity vectors
         * 
         */
        static double calculateDriftCourse(
                double lat1, double lon1,
                double lat2, double lon2,
                double time_delta,
                double relative_speed,
                double heading);

    private:
        /**
         * Calculate great circle distance between two points using Haversine formula
         * 
         */
        static double haversine_distance(double lat1, double lon1, double lat2, double lon2);
};