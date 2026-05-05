#pragma once

#include <string_view>
#include "../../globals/data_types/sentence_data_types/sentence_data_type.h"

struct Parser {
    public:
        std::string_view sentence_sv;
        Parser(const char *input);

        void verify_checksum(); 
        std::string_view next_token() noexcept; 
        DegDecMin parse_latitude();
        DegDecMin parse_longitude();
        double parse_heading(size_t digits);
        double parse_relative_speed();
        double parse_motion();
        std::string_view parse_next_dec(size_t digits);
        char parse_expect_char(char expect1);
        char parse_expect_chars(char expect1, char expect2);
        std::string_view parse_checksum();
        bool has_next() const;
};