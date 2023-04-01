#include <gtest/gtest.h>
#include "../episode.hpp"
#include "../episode_row.hpp"
#include "../string_lookup.hpp"
#include "../config.hpp"


TEST(ParserBoundaries, G069) {
    auto lookup{new_string_lookup()};
    auto config{load_config_file("../../config.yaml")};
    auto parser{new_clinical_code_parser(config["parser"], lookup)};
    auto code{parser->parse(CodeType::Procedure, "G069 ")};
    print(code, lookup);
}
