#pragma once

#include <mutex>
#include <cmath>
#include <deque>
#include <condition_variable>
#include <optional>
#include "./sentence_data_types/sentence_data_type.h"
#include "../../utils/drift_calculator_util/drift_calculator_util.h"
#include "../../utils/converter_util/converter_util.h"

# define KNOTS_PER_MS 0.514444

struct NavigationStateForClient {
    template <typename T>
        struct Field {
            T value{};
            bool is_valid = false;
        };

    Field<double> latitude_dd;
    Field<double> longitude_dd;
    Field<double> heading_rad;
    Field<double> relative_speed_knots;
    Field<double> roll_rad;
    Field<double> pitch_rad;
    Field<double> drift_speed_mps;
    Field<double> drift_course_mps;
};

class NavigationStateBuffer {
    private:
        std::deque<NavigationStateForClient> buffer;
        const size_t max_size = 10;
        mutable std::mutex mutex;
        std::condition_variable cv;

    public:
        void push(const NavigationStateForClient& state) {
            std::lock_guard<std::mutex> lock(mutex);
            buffer.push_back(state);

            if (buffer.size() > max_size) {
                buffer.pop_front();
            }
            cv.notify_all();
        }

        std::optional<NavigationStateForClient> get_last() const {
            std::lock_guard<std::mutex> lock(mutex);
            if (buffer.empty()) return std::nullopt;
            return buffer.back();
        }

        bool wait_for_new_data(std::chrono::seconds timeout = std::chrono::seconds(5)) {
            std::unique_lock<std::mutex> lock(mutex);
            return cv.wait_for(lock, timeout, [this]() { return !buffer.empty(); });
        }
};


template <typename T>
struct TimedValue {
    T value;
    std::chrono::steady_clock::time_point last_update;
    bool valid = false;
};

struct LatitudeSources {
    TimedValue<DegDecMin> fromGP;
    TimedValue<DegDecMin> fromGS;
};

struct LongitudeSources {
    TimedValue<DegDecMin> fromGP;
    TimedValue<DegDecMin> fromGS;
};

struct HeadingSources {
    TimedValue<double> fromPA;
    TimedValue<double> fromHE;
    TimedValue<double> fromVE;
};

struct CurrentNavigationState {
    public:
        CurrentNavigationState() = default;

        void update_position_from_GP(std::chrono::steady_clock::time_point update_time, DegDecMin latitude, DegDecMin longitude) {
            std::lock_guard<std::mutex> lock(mutex);

            this->latitude.fromGP.last_update = update_time;
            this->latitude.fromGP.value = latitude;
            this->latitude.fromGP.valid = true;

            this->longitude.fromGP.last_update = update_time;
            this->longitude.fromGP.value = longitude;
            this->longitude.fromGP.valid = true;
        }

        void update_position_from_GS(std::chrono::steady_clock::time_point update_time, DegDecMin latitude, DegDecMin longitude) {
            std::lock_guard<std::mutex> lock(mutex);

            this->latitude.fromGS.last_update = update_time;
            this->latitude.fromGS.value = latitude;
            this->latitude.fromGS.valid = true;

            this->longitude.fromGS.last_update = update_time;
            this->longitude.fromGS.value = longitude;
            this->longitude.fromGS.valid = true;
        }

        void update_heading_from_PA(std::chrono::steady_clock::time_point update_time, double heading_degree) {
            std::lock_guard<std::mutex> lock(mutex);
            this->heading.fromPA.last_update = update_time;
            this->heading.fromPA.value = ConverterUtil::degrees_to_radians(heading_degree);
            this->heading.fromPA.valid = true;
        }

        void update_heading_from_HE(std::chrono::steady_clock::time_point update_time, double heading_degree) {
            std::lock_guard<std::mutex> lock(mutex);
            this->heading.fromHE.last_update = update_time;
            this->heading.fromHE.value = ConverterUtil::degrees_to_radians(heading_degree);
            this->heading.fromHE.valid = true;
        }

        void update_heading_from_VE(std::chrono::steady_clock::time_point update_time, double heading_degree) {
            std::lock_guard<std::mutex> lock(mutex);
            this->heading.fromVE.last_update = update_time;
            this->heading.fromVE.value = ConverterUtil::degrees_to_radians(heading_degree);
            this->heading.fromVE.valid = true;
        }

        void update_relative_speed(std::chrono::steady_clock::time_point update_time, double speed_knots) {
            std::lock_guard<std::mutex> lock(mutex);
            this->relative_speed.last_update = update_time;
            this->relative_speed.value = speed_knots * KNOTS_PER_MS; 
            this->relative_speed.valid = true;
        }

        void update_pitch(std::chrono::steady_clock::time_point update_time, double pitch_degree) {
            std::lock_guard<std::mutex> lock(mutex);
            this->pitch.last_update = update_time;
            this->pitch.value = ConverterUtil::degrees_to_radians(pitch_degree);
            this->pitch.valid = true;
        }

        void update_roll(std::chrono::steady_clock::time_point update_time, double roll_degree) {
            std::lock_guard<std::mutex> lock(mutex);
            this->roll.last_update = update_time;
            this->roll.value = ConverterUtil::degrees_to_radians(roll_degree);
            this->roll.valid = true;
        }

        NavigationStateForClient generate_navigation_for_client(std::chrono::steady_clock::time_point now, NavigationStateBuffer &navigation_state_buffer, size_t elapsed_time) {
            constexpr auto GP_PERIOD = std::chrono::seconds(1);
            constexpr auto GS_PERIOD = std::chrono::seconds(2);
            constexpr auto HE_PERIOD = std::chrono::seconds(2);
            constexpr auto VE_PERIOD = std::chrono::seconds(2);
            constexpr auto PA_PERIOD = std::chrono::seconds(1);

            NavigationStateForClient result;

            // Latitude 
            // ========================
            if (this->latitude.fromGP.valid && (now - this->latitude.fromGP.last_update <= GP_PERIOD) ) {
                result.latitude_dd.value = this->latitude.fromGP.value.get_decimal_degree();
                result.latitude_dd.is_valid = true;
            }
            else if (this->latitude.fromGS.valid && (now - this->latitude.fromGS.last_update <= GS_PERIOD)) {
                result.latitude_dd.value = this->latitude.fromGS.value.get_decimal_degree();
                result.latitude_dd.is_valid = true;
            } else {
                result.latitude_dd.is_valid = false;
            }

            // Longitude 
            // ========================
            if (this->longitude.fromGP.valid && (now - this->longitude.fromGP.last_update <= GP_PERIOD) ) {
                result.longitude_dd.value = this->longitude.fromGP.value.get_decimal_degree();
                result.longitude_dd.is_valid = true;
            }
            else if (this->longitude.fromGS.valid && (now - this->longitude.fromGS.last_update <= GS_PERIOD)) {
                result.longitude_dd.value = this->longitude.fromGS.value.get_decimal_degree();
                result.longitude_dd.is_valid = true;
            } else {
                result.longitude_dd.is_valid = false;
            }

            // Heading 
            // ========================
            if (this->heading.fromPA.valid && (now - this->heading.fromPA.last_update <= PA_PERIOD)) {
                result.heading_rad.value = this->heading.fromPA.value;
                result.heading_rad.is_valid = true;
            }
            else if (this->heading.fromHE.valid && (now - this->heading.fromHE.last_update <= HE_PERIOD)) {
                result.heading_rad.value = this->heading.fromHE.value;
                result.heading_rad.is_valid = true;
            }
            else if (this->heading.fromVE.valid && (now - this->heading.fromVE.last_update <= VE_PERIOD)) {
                result.heading_rad.value = this->heading.fromVE.value;
                result.heading_rad.is_valid = true;
            } else {
                result.heading_rad.is_valid = false;
            }

            // Relative Speed 
            // ========================
            if (this->relative_speed.valid && (now - this->relative_speed.last_update <= VE_PERIOD)) {
                result.relative_speed_knots.value = this->relative_speed.value;
                result.relative_speed_knots.is_valid = true;
            } else {
                result.relative_speed_knots.is_valid = false;
            }

            // Roll 
            // ========================
            if (this->roll.valid && (now - this->roll.last_update <= PA_PERIOD)) {
                result.roll_rad.value = this->roll.value;
                result.roll_rad.is_valid = true;
            } else {
                result.roll_rad.is_valid = false;
            }

            // Pitch 
            // ========================
            if (this->pitch.valid && (now - this->pitch.last_update <= PA_PERIOD)) {
                result.pitch_rad.value = this->pitch.value;
                result.pitch_rad.is_valid = true;
            } else {
                result.pitch_rad.is_valid = false;
            }

            auto prev_navigation_state = navigation_state_buffer.get_last();
            if (prev_navigation_state.has_value()) {
                auto prev_payload = prev_navigation_state.value();
                if (
                        !prev_payload.latitude_dd.is_valid 
                        || !prev_payload.longitude_dd.is_valid

                        || !result.latitude_dd.is_valid 
                        || !result.longitude_dd.is_valid
                        || !result.relative_speed_knots.is_valid
                        || !result.heading_rad.is_valid
                   ) {
                    result.drift_speed_mps.is_valid = false;
                    result.drift_course_mps.is_valid = false;
                } else {
                    result.drift_course_mps.value = DriftCalculator::calculateDriftCourse(
                            prev_payload.latitude_dd.value,
                            prev_payload.longitude_dd.value,
                            result.latitude_dd.value,
                            result.longitude_dd.value,
                            elapsed_time,
                            result.relative_speed_knots.value,
                            result.heading_rad.value
                            );
                    result.drift_course_mps.is_valid = true;
                    result.drift_speed_mps.value = DriftCalculator::calculateDriftSpeed(
                            prev_payload.latitude_dd.value,
                            prev_payload.longitude_dd.value,
                            result.latitude_dd.value,
                            result.longitude_dd.value,
                            elapsed_time,
                            result.relative_speed_knots.value * KNOTS_PER_MS,
                            result.heading_rad.value
                            );
                    result.drift_speed_mps.is_valid = true;
                }
            } else {
                result.drift_speed_mps.is_valid = false;
                result.drift_course_mps.is_valid = false;
            }

            return result;
        }

        void update(const Sentence& sentence) {
            std::visit([this](const auto& data) {
                const auto now = std::chrono::steady_clock::now();
                using T = std::decay_t<decltype(data)>;

                if constexpr (std::is_same_v<T, GP_Sentence>) {
                    this->update_position_from_GP(now, data.latitude_ddm, data.longitude_ddm);
                }
                else if constexpr (std::is_same_v<T, GS_Sentence>) {
                    this->update_position_from_GP(now, data.latitude_ddm, data.longitude_ddm);
                }
                else if constexpr (std::is_same_v<T, PA_Sentence>) {
                    this->update_heading_from_PA(now, data.heading_deg);
                    this->update_pitch(now, data.pitch_deg);
                    this->update_roll(now, data.roll_deg);
                }
                else if constexpr (std::is_same_v<T, HE_Sentence>) {
                    this->update_heading_from_HE(now, data.heading_deg);
                }
                else if constexpr (std::is_same_v<T, VE_Sentence>) {
                    this->update_relative_speed(now, data.speed_kmph);
                    this->update_heading_from_VE(now, data.heading_deg);
                }
            }, sentence.payload);
        }


    private:

        mutable std::mutex mutex;

        LatitudeSources latitude;
        LongitudeSources longitude;
        HeadingSources heading;

        TimedValue<double> relative_speed;
        TimedValue<double> roll;
        TimedValue<double> pitch;
};

struct AppState {
    CurrentNavigationState navigation_state{};
};