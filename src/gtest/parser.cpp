#include <gtest/gtest.h>
#include "../episode.hpp"
#include "../episode_row.hpp"
#include "../string_lookup.hpp"
#include "../config.hpp"

/// This test currently fails because the file format
/// is wrong (G06 is currently not ordered properly,
/// opcs4.yaml needs fixing). The issue is that
/// the categories are not ordered properly.
TEST(ParserBoundaries, G069) {
    auto lookup{new_string_lookup()};
    auto config{load_config_file("../../config.yaml")};
    auto parser{new_clinical_code_parser(config["parser"], lookup)};
    auto code{parser->parse(CodeType::Procedure, "G069 ")};
    EXPECT_TRUE(code.valid());
}

/// Also incorrect index in file
TEST(ParserBoundaries, W561) {
    auto lookup{new_string_lookup()};
    auto config{load_config_file("../../config.yaml")};
    auto parser{new_clinical_code_parser(config["parser"], lookup)};
    auto code{parser->parse(CodeType::Procedure, "  W56.1 ")};
    EXPECT_TRUE(code.valid());
}

/// Also incorrect index in file
TEST(ParserBoundaries, W983) {
    auto lookup{new_string_lookup()};
    auto config{load_config_file("../../config.yaml")};
    auto parser{new_clinical_code_parser(config["parser"], lookup)};
    auto code{parser->parse(CodeType::Procedure, "W983")};
    EXPECT_TRUE(code.valid());
}
