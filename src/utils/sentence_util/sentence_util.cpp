
    #include "sentence_util.h"
    #include "iostream"
    #include "../parser_util/parser_util.h"

    Sentence SentenceUtil::parse_from_char(char *raw_data)
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