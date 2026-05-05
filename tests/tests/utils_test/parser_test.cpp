// Using Google Test library
#include <gtest/gtest.h>
#include <charconv>
#include <cmath>

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

        // std::cout << checksum << std::endl;
        // std::cout << payload << std::endl;

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

        // std::cout << token << std::endl;

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

#define M_PI 3.14159265358979323846

// Include or copy your Parser class here
#include <string_view>
#include <charconv>
#include <stdexcept>

// ============================================================================
// UNIT TESTS
// ============================================================================

// Test GP sentence parsing
TEST(ParserTest, ParseGP_Valid) {
    const char* sentence = "$GP,4807.038,N,01131.000,E*2E";
    Parser p(sentence);
    
    ASSERT_NO_THROW(p.verify_checksum());
    
    auto type = p.next_token();
    EXPECT_EQ(type, "GP");
    
    auto lat = p.parse_latitude();
    EXPECT_EQ(lat.degrees, 48);
    EXPECT_DOUBLE_EQ(lat.minutes, 7.038);
    EXPECT_EQ(lat.direction, 'N');
    
    auto lon = p.parse_longitude();
    EXPECT_EQ(lon.degrees, 11);
    EXPECT_DOUBLE_EQ(lon.minutes, 31.000);
    EXPECT_EQ(lon.direction, 'E');
}

// Test GS sentence parsing
TEST(ParserTest, ParseGS_Valid) {
    const char* sentence = "$GS,01131.000,E,4807.038,N*2D";
    Parser p(sentence);
    
    ASSERT_NO_THROW(p.verify_checksum());
    
    auto type = p.next_token();
    EXPECT_EQ(type, "GS");
    
    auto lon = p.parse_longitude();
    EXPECT_EQ(lon.degrees, 11);
    EXPECT_DOUBLE_EQ(lon.minutes, 31.000);
    EXPECT_EQ(lon.direction, 'E');
    
    auto lat = p.parse_latitude();
    EXPECT_EQ(lat.degrees, 48);
    EXPECT_DOUBLE_EQ(lat.minutes, 7.038);
    EXPECT_EQ(lat.direction, 'N');
}

// Test HE sentence parsing
TEST(ParserTest, ParseHE_Valid) {
    const char* sentence = "$HE,123.456,T*70";
    Parser p(sentence);
    
    ASSERT_NO_THROW(p.verify_checksum());
    
    auto type = p.next_token();
    EXPECT_EQ(type, "HE");
    
    double heading = p.parse_heading(3);
    EXPECT_DOUBLE_EQ(heading, 123.456);
}

// Test VE sentence parsing
TEST(ParserTest, ParseVE_Valid) {
    const char* sentence = "$VE,15.0,K,8.1,N,45.23,T*51";
    Parser p(sentence);
    
    ASSERT_NO_THROW(p.verify_checksum());
    
    auto type = p.next_token();
    EXPECT_EQ(type, "VE");
    
    double speed_kmh = p.parse_relative_speed();
    EXPECT_DOUBLE_EQ(speed_kmh, 15.0);
    
    char unit1 = p.parse_expect_char('K');
    EXPECT_EQ(unit1, 'K');
    
    double speed_knots = p.parse_relative_speed();
    EXPECT_DOUBLE_EQ(speed_knots, 8.1);
    
    char unit2 = p.parse_expect_char('N');
    EXPECT_EQ(unit2, 'N');
    
    double heading = p.parse_heading(2);
    EXPECT_DOUBLE_EQ(heading, 45.23);
}

// Test checksum validation
TEST(ParserTest, Checksum_Valid) {
    const char* sentence = "$GP,4807.038,N,01131.000,E*2E";
    Parser p(sentence);
    
    ASSERT_NO_THROW(p.verify_checksum());
}

TEST(ParserTest, Checksum_Invalid) {
    const char* sentence = "$GP,4807.038,N,01131.000,E*FF"; // Wrong checksum
    Parser p(sentence);
    
    EXPECT_THROW(p.verify_checksum(), std::runtime_error);
}

TEST(ParserTest, Checksum_Missing) {
    const char* sentence = "$GP,4807.038,N,01131.000,E"; // No checksum
    Parser p(sentence);
    
    EXPECT_THROW(p.verify_checksum(), std::invalid_argument);
}

TEST(ParserTest, Checksum_MissingDollar) {
    const char* sentence = "GP,4807.038,N,01131.000,E*2E"; // No $
    Parser p(sentence);
    
    EXPECT_THROW(p.verify_checksum(), std::runtime_error);
}

// Test DDM to decimal conversion
TEST(ParserTest, DDM_ToDecimal_North) {
    DegDecMin coord{48, 7.038, 'N'};
    double decimal = coord.get_decimal_degree();
    EXPECT_NEAR(decimal, 48.1173, 0.0001);
}

TEST(ParserTest, DDM_ToDecimal_South) {
    DegDecMin coord{30, 45.123, 'S'};
    double decimal = coord.get_decimal_degree();
    EXPECT_NEAR(decimal, -(30 + 45.123/60.0), 0.0001);
}

TEST(ParserTest, DDM_ToDecimal_East) {
    DegDecMin coord{11, 31.000, 'E'};
    double decimal = coord.get_decimal_degree();
    EXPECT_NEAR(decimal, 11.5167, 0.0001);
}

TEST(ParserTest, DDM_ToDecimal_West) {
    DegDecMin coord{150, 30.456, 'W'};
    double decimal = coord.get_decimal_degree();
    EXPECT_NEAR(decimal, -(150 + 30.456/60.0), 0.0001);
}

// Test tokenization
TEST(ParserTest, NextToken_Basic) {
    const char* sentence = "GP,4807.038,N";
    Parser p(sentence);
    
    EXPECT_EQ(p.next_token(), "GP");
    EXPECT_EQ(p.next_token(), "4807.038");
    EXPECT_EQ(p.next_token(), "N");
    EXPECT_EQ(p.next_token(), ""); // Empty when no more tokens
}

// Main function
int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}