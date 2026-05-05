#pragma once

#include "../../globals/data_types/sentence_data_types/sentence_data_type.h"

class SentenceUtil {
    public:
        static Sentence parse_from_char(char *raw_data);
};
