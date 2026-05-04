#include <csignal>
#include <iostream>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <variant>
#include <string_view>
#include <charconv>
#include <assert.h>
#include <atomic>
#include <cmath>
#include <deque>
#include <optional>
#include <grpc++/grpc++.h>
#include <shared_mutex>
#include <thread>
#include "utils/message_queue/message_queue.inl"

# define KNOTS_PER_MS 0.514444

const char* IP = "127.0.0.1";
constexpr int PORT = 5000;

struct DegDecMin {
    int degrees;
    double minutes;
    char direction;

    double get_decimal_degree() {
        auto result = degrees + (minutes / 60);
        if (direction == 'S' || direction == 'W') 
            result *= -1;

        return result;
    }
};

double degrees_to_radians(double degree) {
    return degree * (M_PI/180);
}

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

class DriftCalculator {
    public:
        /**
         * Calculate absolute speed (speed over ground) from position changes
         */
        static double calculate_absolute_speed(
                double lat1, double lon1,
                double lat2, double lon2,
                double time_delta) {

            if (time_delta <= 0.0) {
                return 0.0;
            }

            double distance = haversine_distance(lat1, lon1, lat2, lon2);

            return distance / time_delta;  // m/s
        }

        /**
         * Calculate absolute course (course over ground) from position changes
         * 
         */   
        static double calculateAbsoluteCourse(
                double lat1, double lon1,
                double lat2, double lon2) {

            // Convert to radians
            double lat1_rad = degrees_to_radians(lat1);
            double lon1_rad = degrees_to_radians(lon1);
            double lat2_rad = degrees_to_radians(lat2);
            double lon2_rad = degrees_to_radians(lon2);

            double dlon = lon2_rad - lon1_rad;

            double y = std::sin(dlon) * std::cos(lat2_rad);
            double x = std::cos(lat1_rad) * std::sin(lat2_rad) -
                std::sin(lat1_rad) * std::cos(lat2_rad) * std::cos(dlon);

            double bearing = std::atan2(y, x);

            bearing = std::fmod(bearing + 2 * M_PI, 2 * M_PI);

            return bearing;  // radians
        }

        /**
         * Calculate drift speed from absolute and relative speeds
         * 
         */
        static double calculateDriftSpeed(
                double lat1, double lon1,
                double lat2, double lon2,
                double time_delta,
                double relative_speed,
                double heading) {

            auto absolute_speed = calculate_absolute_speed(lat1, lon1, lat2, lon2, time_delta);
            auto absolute_course = calculateAbsoluteCourse(lat1, lon1, lat2, lon2);

            if (absolute_speed < 0.0 || relative_speed < 0.0) {
                return 0.0;
            }

            double angle_diff = absolute_course - heading;

            double drift_speed_squared = 
                absolute_speed * absolute_speed +
                relative_speed * relative_speed -
                2.0 * absolute_speed * relative_speed * std::cos(angle_diff);

            if (drift_speed_squared < 0.0) {
                drift_speed_squared = 0.0;
            }

            return std::sqrt(drift_speed_squared);  // m/s
        }

        /**
         * Calculate drift course from velocity vectors
         * 
         */
        static double calculateDriftCourse(
                double lat1, double lon1,
                double lat2, double lon2,
                double time_delta,
                double relative_speed,
                double heading) {

            auto absolute_speed = calculate_absolute_speed(lat1, lon1, lat2, lon2, time_delta);
            auto absolute_course = calculateAbsoluteCourse(lat1, lon1, lat2, lon2);

            if (absolute_speed < 0.0 || relative_speed < 0.0) {
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

            return drift_course;  // radians
        }

    private:
        /**
         * Calculate great circle distance between two points using Haversine formula
         * 
         */
        static double haversine_distance(double lat1, double lon1, double lat2, double lon2) {
            const double R = 6371000.0;  // Earth's radius in meters

            // Convert to radians
            double lat1_rad = degrees_to_radians(lat1);
            double lon1_rad = degrees_to_radians(lon1);
            double lat2_rad = degrees_to_radians(lat2);
            double lon2_rad = degrees_to_radians(lon2);

            double dlat = lat2_rad - lat1_rad;
            double dlon = lon2_rad - lon1_rad;

            double a = std::sin(dlat / 2.0) * std::sin(dlat / 2.0) +
                std::cos(lat1_rad) * std::cos(lat2_rad) *
                std::sin(dlon / 2.0) * std::sin(dlon / 2.0);

            double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));

            return R * c;  // meters
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

enum SentenceType {
    GP,
    GS,
    HE,
    VE,
    PA,
    Unknown
};

struct GP_Sentence {
    DegDecMin latitude_ddm;
    DegDecMin longitude_ddm;
};

struct GS_Sentence {
    DegDecMin latitude_ddm;
    DegDecMin longitude_ddm;
};

struct HE_Sentence {
    double heading_deg;
};

struct VE_Sentence {
    double speed_kmph;
    double speed_knots;
    double heading_deg;
};

struct PA_Sentence {
    double heading_deg;
    double pitch_deg;
    double roll_deg;
};

using SentencePayload = std::variant<GP_Sentence, GS_Sentence, HE_Sentence, VE_Sentence, PA_Sentence>;

struct Parser {
    std::string_view sentence_sv;

    Parser(const char *input): sentence_sv(input) {}

    void verify_checksum() {
        if (!has_next()) throw std::runtime_error("Invalid input when verifying checksum");

        if (sentence_sv.empty() || sentence_sv.front() != '$')
            throw std::runtime_error("Sentence must start with '$'");

        auto star_pos = sentence_sv.find('*');
        if (star_pos == std::string_view::npos) throw std::invalid_argument("Parse failure: invalid data sentence");

        auto payload = sentence_sv.substr(1, star_pos - 1);
        std::string_view checksum = sentence_sv.substr(star_pos + 1);
        if (checksum.size() != 2)
            throw std::runtime_error("Invalid checksum length");

        uint8_t computed = 0;
        for (char c : payload) {
            computed ^= static_cast<uint8_t>(c);
        }

        uint8_t received{};
        auto [ptr, ec] = std::from_chars(
                checksum.data(),
                checksum.data() + checksum.size(),
                received,
                16
                );

        if (ec != std::errc{} || ptr != checksum.data() + checksum.size())
            throw std::runtime_error("Invalid checksum format");

        if (computed != received)
            throw std::runtime_error("Checksum mismatch");

        sentence_sv = payload;
    }

    std::string_view next_token() noexcept {
        if (!has_next()) return {};

        const char *curr = sentence_sv.data();
        const char *start = curr;
        const char* end  = sentence_sv.data() + sentence_sv.size();

        while(curr < end && *curr != ',') {
            ++curr;
        };

        auto result =  std::string_view(start, curr - start);

        if (curr < end && *curr == ',') {
            ++curr;
        }

        sentence_sv = std::string_view(curr, end - curr);
        return result;
    }

    DegDecMin parse_latitude() {
        std::string_view latitude = parse_next_dec(3);

        if (latitude.size() < 4)
            throw std::runtime_error("Latitude too short");

        // Degrees (2 digits)
        std::string_view deg_sv = latitude.substr(0, 2);
        int degrees{};
        auto [p1, ec1] = std::from_chars(deg_sv.data(),
                deg_sv.data() + deg_sv.size(),
                degrees);
        if (ec1 != std::errc{} || p1 != deg_sv.data() + deg_sv.size())
            throw std::runtime_error("Invalid latitude degrees format");
        if (degrees < 0 || degrees > 89)
            throw std::runtime_error("Latitude degrees out of range");

        // Minutes
        std::string_view min_sv = latitude.substr(2);
        double minutes{};

        if (min_sv.find('.') == std::string_view::npos)
            throw std::runtime_error("Minutes must contain decimal point");

        auto [p2, ec2] = std::from_chars(min_sv.data(),
                min_sv.data() + min_sv.size(),
                minutes);
        if (ec2 != std::errc{} || p2 != min_sv.data() + min_sv.size())
            throw std::runtime_error("Invalid latitude minutes format");
        if (minutes < 0.0 || minutes >= 60.0)
            throw std::runtime_error("Latitude minutes out of range");

        char direction = parse_expect_chars('N', 'S');

        auto result = DegDecMin{
            .degrees = degrees,
                .minutes = minutes,
                .direction = direction,
        };
        return result;
    }

    DegDecMin parse_longitude() {
        std::string_view longitude = parse_next_dec(3);

        if (longitude.size() < 4)
            throw std::runtime_error("Longitude too short");

        // Degrees (3 digits)
        std::string_view deg_sv = longitude.substr(0, 3);
        int degrees{};
        auto [p1, ec1] = std::from_chars(deg_sv.data(),
                deg_sv.data() + deg_sv.size(),
                degrees);
        if (ec1 != std::errc{} || p1 != deg_sv.data() + deg_sv.size())
            throw std::runtime_error("Invalid longitude degrees format");
        if (degrees < 0 || degrees > 179)
            throw std::runtime_error("Longitude degrees out of range");

        // Minutes
        std::string_view min_sv = longitude.substr(3);
        double minutes{};

        if (min_sv.find('.') == std::string_view::npos)
            throw std::runtime_error("Minutes must contain decimal point");

        auto [p2, ec2] = std::from_chars(min_sv.data(),
                min_sv.data() + min_sv.size(),
                minutes);
        if (ec2 != std::errc{} || p2 != min_sv.data() + min_sv.size())
            throw std::runtime_error("Invalid latitude minutes format");
        if (minutes < 0.0 || minutes >= 60.0)
            throw std::runtime_error("Latitude minutes out of range");

        char direction = parse_expect_chars('E', 'W');

        auto result = DegDecMin{
            .degrees = degrees,
                .minutes = minutes,
                .direction = direction,
        };
        return result;
    }

    double parse_heading(size_t digits) {
        std::string_view heading_sv = parse_next_dec(digits);
        double heading{};
        auto [p1, ec1] = std::from_chars(heading_sv.data(),
                heading_sv.data() + heading_sv.size(),
                heading);
        if (ec1 != std::errc{} || p1 != heading_sv.data() + heading_sv.size())
            throw std::runtime_error("Invalid heading format");
        if (heading < -180 || heading > 179.99)
            throw std::runtime_error("Heading degrees out of range");

        parse_expect_char('T');
        return heading;
    }

    double parse_relative_speed() {
        std::string_view speed_sv = parse_next_dec(1);

        double speed{};
        auto [p1, ec1] = std::from_chars(speed_sv.data(),
                speed_sv.data() + speed_sv.size(),
                speed);
        if (ec1 != std::errc{} || p1 != speed_sv.data() + speed_sv.size())
            throw std::runtime_error("Invalid heading format");
        if (speed < -180 || speed > 179.99)
            throw std::runtime_error("Heading degrees out of range");

        return speed;
    }

    double parse_motion() {
        std::string_view motion_sv = parse_next_dec(1);
        double motion{};
        auto [p1, ec1] = std::from_chars(motion_sv.data(),
                motion_sv.data() + motion_sv.size(),
                motion);
        if (ec1 != std::errc{} || p1 != motion_sv.data() + motion_sv.size())
            throw std::runtime_error("Invalid heading format");
        if (motion < -180 || motion > 179.9)
            throw std::runtime_error("Heading degrees out of range");

        return motion;
    }

    std::string_view parse_next_dec(size_t digits) {
        std::string_view token = next_token();
        if (token.empty()) throw std::invalid_argument("Parse failure: Empty char");

        auto dot_pos = token.find('.');
        if (dot_pos == std::string_view::npos) throw std::invalid_argument("Parse failure: invalid dec");

        const size_t token_size = token.size();
        if ((token_size - 1 - dot_pos) != digits){
            throw std::invalid_argument("Parse failure: invalid decimal digits count");
        }

        for (const auto &chr: token) {
            if (chr == '.' || '-') continue;

            if (!std::isdigit(chr))
                throw std::invalid_argument("Parse failure: invalid decimal digits");
        }

        return token;
    }

    char parse_expect_char(char expect1) {
        std::string_view token = next_token();

        if (token.empty()) throw std::invalid_argument("Parse failure: Empty char");
        if (token.size() != 1) throw std::invalid_argument("Parse failure: invalid char expect");

        if (token[0] == expect1) return expect1;

        throw std::invalid_argument("Parse failure: invalid char expect");
    }

    char parse_expect_chars(char expect1, char expect2) {
        std::string_view token = next_token();

        if (token.empty()) throw std::invalid_argument("Parse failure: Empty char");
        if (token.size() != 1) throw std::invalid_argument("Parse failure: invalid char expect");

        if (token[0] == expect1) return expect1;
        if (token[0] == expect2) return expect2;

        throw std::invalid_argument("Parse failure: invalid char expect");
    }

    std::string_view parse_checksum() {
        std::string_view checksum = next_token();
        if (checksum.empty()) throw std::invalid_argument("Parse failure: Empty char");

        return checksum;
    }

    bool has_next() const {
        return sentence_sv.data() && *sentence_sv.data() != '\0';
    }
};

struct Sentence {
    SentenceType type;
    SentencePayload payload;

    static Sentence parse_from_char(char *raw_data)
    {
        Parser p(raw_data);

        try
        {
            p.verify_checksum();
            std::string_view token_type = p.next_token();

            if (token_type == "GP")
            {
                DegDecMin latitude = p.parse_latitude();
                DegDecMin longitude = p.parse_longitude();

                std::cout << "GP : " << latitude.degrees << " , " << latitude.minutes << " , " << latitude.direction << " , " << std::endl;
                std::cout << longitude.degrees << " , " << longitude.minutes << " , " << longitude.direction << std::endl;

                return Sentence{
                    .type = SentenceType::GP,
                    .payload = GP_Sentence {
                        .latitude_ddm = latitude,
                        .longitude_ddm = longitude,
                    },
                };
            }
            else if (token_type == "GS")
            {
                auto longitude = p.parse_longitude();
                auto latitude = p.parse_latitude();

                std::cout << "GS : " << latitude.degrees << " , " << latitude.minutes << " , " << latitude.direction << " , " << std::endl;
                std::cout << longitude.degrees << " , " << longitude.minutes << " , " << longitude.direction << std::endl;

                return Sentence {
                    .type = SentenceType::GS,
                    .payload = GS_Sentence {
                        .latitude_ddm = latitude,
                        .longitude_ddm = longitude,
                    }
                };
            }
            else if (token_type == "HE")
            {
                double heading_degree = p.parse_heading(3);
                std::cout << "HE : " << heading_degree << std::endl;
                return Sentence {
                    .type = SentenceType::HE, 
                    .payload = HE_Sentence {
                        .heading_deg = heading_degree
                    }
                };
            }
            else if (token_type == "VE")
            {
                double relative_speed_km_h = p.parse_relative_speed();
                p.parse_expect_char('K');

                double relative_speed_knots = p.parse_relative_speed();
                p.parse_expect_char('N');

                double heading_degree = p.parse_heading(2);
                std::cout << "VE : " << relative_speed_km_h << " , " << relative_speed_knots << " , " << heading_degree << std::endl;
                // NOTE(wesly): For now just use relative speed in knots
                return Sentence{
                    .type = SentenceType::VE,
                    .payload = VE_Sentence {
                        .speed_kmph = relative_speed_km_h,
                        .speed_knots = relative_speed_knots,
                        .heading_deg = heading_degree,
                    }
                };
            }
            else if (token_type == "PA")
            {
                double heading_degree = p.parse_heading(3);
                double pitch_degree = p.parse_motion();
                double roll_degree = p.parse_motion();

                std::cout << "PA : " << heading_degree << " , " << pitch_degree << " , " << roll_degree << std::endl;
                return Sentence {
                    .type = SentenceType::PA,
                    .payload = PA_Sentence {
                        .heading_deg = heading_degree,
                        .pitch_deg = pitch_degree,
                        .roll_deg = roll_degree
                    }
                };
            }
            else 
            {
                throw std::runtime_error("Unexpected not recognized sentence");
            }
        }
        catch (std::exception &e)
        {
            std::cerr << e.what() << std::endl;
            // TODO(wesly): Do something when catching an exception
            exit(69);
        }
    }
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
            this->heading.fromPA.value = degrees_to_radians(heading_degree);
            this->heading.fromPA.valid = true;
        }

        void update_heading_from_HE(std::chrono::steady_clock::time_point update_time, double heading_degree) {
            std::lock_guard<std::mutex> lock(mutex);
            this->heading.fromHE.last_update = update_time;
            this->heading.fromHE.value = degrees_to_radians(heading_degree);
            this->heading.fromHE.valid = true;
        }

        void update_heading_from_VE(std::chrono::steady_clock::time_point update_time, double heading_degree) {
            std::lock_guard<std::mutex> lock(mutex);
            this->heading.fromVE.last_update = update_time;
            this->heading.fromVE.value = degrees_to_radians(heading_degree);
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
            this->pitch.value = degrees_to_radians(pitch_degree);
            this->pitch.valid = true;
        }

        void update_roll(std::chrono::steady_clock::time_point update_time, double roll_degree) {
            std::lock_guard<std::mutex> lock(mutex);
            this->roll.last_update = update_time;
            this->roll.value = degrees_to_radians(roll_degree);
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
    // More...
};

struct ThreadSyncPrimitive {
    std::shared_mutex navigation_state_mtx{};
};

class InterfaceReceiver {
    public:
        virtual ~InterfaceReceiver() = default;

        virtual void start() = 0;
        virtual void stop() = 0;
};

class Observer {
    public:
        virtual ~Observer() = default;
        virtual void update_data() = 0;
};

class Observable {
    public:
        void notify_observers() {
            for (auto& observer : this->observers_) {
                this->threads_.emplace_back([observer] () {
                    observer->update_data();
                });
            }
        }

        void sync_threads() {
            for (auto& thread: this->threads_) {
                thread.join();
            }
            this->threads_.clear();
        }

        void add_observer(Observer* observer) {
            this->observers_.push_back(observer);
        }

        virtual void remove_observer(Observer* observer) {
            this->observers_.remove(observer);
        }

    private:
        std::list<Observer*> observers_;
        std::list<std::thread> threads_;
};

class InterfaceRawNavigationStateReceiver : public InterfaceReceiver {
    public:
        ~InterfaceRawNavigationStateReceiver() override = default;

        virtual Sentence get_sentence() = 0;

        virtual void start() = 0;

        virtual void stop() = 0;

        virtual void add_observer(Observer *observer) = 0;
        
        virtual void remove_observer(Observer *observer) = 0;
};

template<typename MessageT>
class DataListener {
public:
    DataListener() = default;

    MessageQueue<MessageT>* get_queue_() {
        return &this->queue_;
    }

    void on_data_available(char* data) {
        Sentence parsed = Sentence::parse_from_char(data);
        this->queue_.push(std::move(parsed));
    }
private:
    MessageQueue<MessageT> queue_{};
};

class RawNavigationStateSubscriber : public Observable, public InterfaceRawNavigationStateReceiver {
    public:
        RawNavigationStateSubscriber() {
            if (this->is_running_) {
                std::cerr << "UDP Listener already running" << std::endl;
                return;
            }

            this->sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
            if (this->sockfd_ < 0) {
                std::cerr << "Failed to create socket" << std::endl;
                return;
            }

            struct timeval tv;
            tv.tv_sec = 1;
            tv.tv_usec = 0;
            setsockopt(this->sockfd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

            struct sockaddr_in server_addr;
            memset(&server_addr, 0, sizeof(server_addr));
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(PORT);

            if (inet_pton(AF_INET, IP, &server_addr.sin_addr) <= 0) {
                std::cerr << "Invalid address" << std::endl;
                close(this->sockfd_);
                this->sockfd_ = -1;
                return;
            }

            if (bind(this->sockfd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                std::cerr << "bind failed" << std::endl;
                close(this->sockfd_);
                this->sockfd_ = -1;
                return;
            }

            this->is_running_ = true;

            std::thread t([this]() {
                struct sockaddr_in sender_addr{};
                socklen_t sender_len = sizeof(sender_addr);
                while (this->is_running_) {
                    char buff[1024];
                    auto received = recvfrom(
                            this->sockfd_, buff, sizeof(buff) - 1,
                            0, (struct sockaddr*)&sender_addr, &sender_len);

                    if (received < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            continue;
                        }

                        std::cerr << "Error receiving UDP packet: " << strerror(errno) << std::endl;
                        continue;
                    }

                    buff[received] = '\0';

                    this->listener_.on_data_available(buff);
                }
            }); t.detach();
            std::cout << "Listening via UDP into : " << IP << ":" << PORT << std::endl;
        }

        void start() override {
            std::cout << "UDP Subscriber [Raw Navigation State ] Ready ..." << std::endl;

            Sentence buff{};
            while (this->listener_.get_queue_()->wait_and_pop(buff)) {
                this->sync_threads();
                this->sentence_ = buff;
                this->notify_observers();
            }
        }

        void stop() {
            this->is_running_ = false;
        }

        void add_observer(Observer* observer) {
            Observable::add_observer(observer);
        }

        void remove_observer(Observer* observer) {
            Observable::remove_observer(observer);
        }

        Sentence get_sentence() {
            return this->sentence_;
        }

    private:
        std::atomic<bool> is_running_ = false;
        int sockfd_{};
        Sentence sentence_{};
        DataListener<Sentence> listener_;
        void process_message(const char* message);
};


class RawNavigationHandler : public Observer {
    public:
        RawNavigationHandler (
            InterfaceRawNavigationStateReceiver *observable,
            AppState &app_state,
            ThreadSyncPrimitive &thread_sync_primitive
        ) : observable_(observable), app_state_(app_state), thread_sync_primitive_(thread_sync_primitive) {}

        void update_data() {
            std::lock_guard(this->thread_sync_primitive_.navigation_state_mtx);
            Sentence new_data = this->observable_->get_sentence();
            this->app_state_.navigation_state.update(new_data);
        }

    private:
        InterfaceRawNavigationStateReceiver* observable_;
        AppState &app_state_;
        ThreadSyncPrimitive &thread_sync_primitive_;
};

class BackendInterfaceThreadsContainer {
    public:
        static void raw_navigation_state_receiver_thread(
            InterfaceRawNavigationStateReceiver *raw_navigation_state_receiver,
            AppState &app_state,
            ThreadSyncPrimitive &thread_sync_primitive
        ) 
        {
            RawNavigationHandler raw_navigation_handler(
                raw_navigation_state_receiver,
                app_state,
                thread_sync_primitive
            );
            raw_navigation_state_receiver->add_observer(&raw_navigation_handler);
            raw_navigation_state_receiver->start();
        }
};

int main() {
    AppState app_state{};

    ThreadSyncPrimitive thread_sync_primitive{};
    RawNavigationStateSubscriber raw_navigation_state_subscriber{};

    std::thread raw_navigation_state_subscriber_thread
    (
        BackendInterfaceThreadsContainer::raw_navigation_state_receiver_thread,
        &raw_navigation_state_subscriber,
        std::ref(app_state),
        std::ref(thread_sync_primitive)
    );

    raw_navigation_state_subscriber_thread.join(); 

    return EXIT_SUCCESS;
}