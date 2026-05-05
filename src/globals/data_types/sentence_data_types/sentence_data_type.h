#pragma once

#include <variant>

struct DegDecMin {
    int degrees{};
    double minutes{};
    char direction{};

    double get_decimal_degree() {
        auto result = degrees + (minutes / 60);
        if (direction == 'S' || direction == 'W') 
            result *= -1;

        return result;
    }
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
    DegDecMin latitude_ddm{};
    DegDecMin longitude_ddm{};
};

struct GS_Sentence {
    DegDecMin latitude_ddm{};
    DegDecMin longitude_ddm{};
};

struct HE_Sentence {
    double heading_deg{};
};

struct VE_Sentence {
    double speed_kmph{};
    double speed_knots{};
    double heading_deg{};
};

struct PA_Sentence {
    double heading_deg{};
    double pitch_deg{};
    double roll_deg{};
};

using SentencePayload = std::variant<GP_Sentence, GS_Sentence, HE_Sentence, VE_Sentence, PA_Sentence>;

struct Sentence {
    SentenceType type{};
    SentencePayload payload{};
};

