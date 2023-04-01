#include <gtest/gtest.h>
#include "../episode.hpp"
#include "../episode_row.hpp"
#include "../string_lookup.hpp"
#include "../config.hpp"

/// This test currently fails because the file format
/// is wrong (G06 is currently not ordered properly,
/// opcs4.yaml needs fixing)
TEST(ParserBoundaries, G069) {
    auto lookup{new_string_lookup()};
    auto config{load_config_file("../../config.yaml")};
    auto parser{new_clinical_code_parser(config["parser"], lookup)};
    auto code{parser->parse(CodeType::Procedure, "G069 ")};
    EXPECT_FALSE(code.invalid());
}
