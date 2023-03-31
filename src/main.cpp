#include <iostream>
//#include "acs.hpp"
#include "clinical_code.hpp"
#include "episode.hpp"
#include "episode_row.hpp"
#include "spell_rows.hpp"
#include "spell.hpp"
#include "config.hpp"
#include "sql_query.hpp"
#include "sql_connection.hpp"

int main() {
    auto lookup{new_string_lookup()};
    auto config{load_config_file("../../config.yaml")};
    auto parser{new_clinical_code_parser(config["parser"], lookup)};
    auto sql_connection{new_sql_connection(config["connection"])};
    auto sql_query{make_acs_sql_query(config["sql_query"], false)};

    std::cout << sql_query << std::endl;
    
    auto row{sql_connection.execute_direct(sql_query)};

    Episode episode{row, parser};
    episode.print(lookup);
    
    // Spell spell{rows, parser};
    // spell.print(parser);    
}
