// Using Google Test library
#include <gtest/gtest.h>
#include <charconv>
#include <cmath>
#include <string_view>
#include <charconv>
#include <stdexcept>

#include "../../../src/utils/parser_util/parser_util.h"

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