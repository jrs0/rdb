#include <iostream>
//#include "acs.hpp"
#include "clinical_code.hpp"
#include "episode.hpp"
#include "episode_row.hpp"
#include "spell_rows.hpp"
#include "spell.hpp"
#include "config.hpp"
#include "sql_query.hpp"

int main() {
    auto lookup{new_string_lookup()};
    auto config{load_config_file("../../config.yaml")};
    auto parser{new_clinical_code_parser(config["parser"], lookup)};

    std::cout << make_acs_sql_query(config["sql_query"]) << std::endl;

    
    // Spell spell{rows, parser};
    // spell.print(parser);    
}
