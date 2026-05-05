#include <stdexcept>
#include <string_view>
#include <charconv>
#include <cstdint>
#include "parser_util.h"

Parser::Parser(const char *input) : sentence_sv(input) {}

void Parser::verify_checksum()
{
    if (!has_next())
        throw std::runtime_error("Invalid input when verifying checksum");

    if (sentence_sv.empty() || sentence_sv.front() != '$')
        throw std::runtime_error("Sentence must start with '$'");

    auto star_pos = sentence_sv.find('*');
    if (star_pos == std::string_view::npos)
        throw std::invalid_argument("Parse failure: invalid data sentence");

    auto payload = sentence_sv.substr(1, star_pos - 1);
    std::string_view checksum = sentence_sv.substr(star_pos + 1);
    if (checksum.size() != 2)
        throw std::runtime_error("Invalid checksum length");

    uint8_t computed = 0;
    for (char c : payload)
    {
        computed ^= static_cast<uint8_t>(c);
    }

    uint8_t received{};
    auto [ptr, ec] = std::from_chars(
        checksum.data(),
        checksum.data() + checksum.size(),
        received,
        16);

    if (ec != std::errc{} || ptr != checksum.data() + checksum.size())
        throw std::runtime_error("Invalid checksum format");

    if (computed != received)
        throw std::runtime_error("Checksum mismatch");

    sentence_sv = payload;
}

DegDecMin Parser::parse_latitude()
{
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

std::string_view Parser::next_token() noexcept
{
    if (!has_next())
        return {};

    const char *curr = sentence_sv.data();
    const char *start = curr;
    const char *end = sentence_sv.data() + sentence_sv.size();

    while (curr < end && *curr != ',')
    {
        ++curr;
    };

    auto result = std::string_view(start, curr - start);

    if (curr < end && *curr == ',')
    {
        ++curr;
    }

    sentence_sv = std::string_view(curr, end - curr);
    return result;
}

DegDecMin Parser::parse_longitude()
{
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

double Parser::parse_heading(size_t digits)
{
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

double Parser::parse_relative_speed()
{
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

double Parser::parse_motion()
{
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

std::string_view Parser::parse_next_dec(size_t digits)
{
    std::string_view token = next_token();
    if (token.empty())
        throw std::invalid_argument("Parse failure: Empty char");

    auto dot_pos = token.find('.');
    if (dot_pos == std::string_view::npos)
        throw std::invalid_argument("Parse failure: invalid dec");

    const size_t token_size = token.size();
    if ((token_size - 1 - dot_pos) != digits)
    {
        throw std::invalid_argument("Parse failure: invalid decimal digits count");
    }

    for (const auto &chr : token)
    {
        if (chr == '.' || '-')
            continue;

        if (!std::isdigit(chr))
            throw std::invalid_argument("Parse failure: invalid decimal digits");
    }

    return token;
}

char Parser::parse_expect_char(char expect1)
{
    std::string_view token = next_token();

    if (token.empty())
        throw std::invalid_argument("Parse failure: Empty char");
    if (token.size() != 1)
        throw std::invalid_argument("Parse failure: invalid char expect");

    if (token[0] == expect1)
        return expect1;

    throw std::invalid_argument("Parse failure: invalid char expect");
}

char Parser::parse_expect_chars(char expect1, char expect2)
{
    std::string_view token = next_token();

    if (token.empty())
        throw std::invalid_argument("Parse failure: Empty char");
    if (token.size() != 1)
        throw std::invalid_argument("Parse failure: invalid char expect");

    if (token[0] == expect1)
        return expect1;
    if (token[0] == expect2)
        return expect2;

    throw std::invalid_argument("Parse failure: invalid char expect");
}

std::string_view Parser::parse_checksum()
{
    std::string_view checksum = next_token();
    if (checksum.empty())
        throw std::invalid_argument("Parse failure: Empty char");

    return checksum;
}

bool Parser::has_next() const
{
    return sentence_sv.data() && *sentence_sv.data() != '\0';
}