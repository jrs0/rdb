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

#include "cmdline.hpp"

int main(int argc, char ** argv) {

    auto lookup{new_string_lookup()};
    auto config{load_config_file("../../config.yaml")};
    auto parser{new_clinical_code_parser(config["parser"], lookup)};

    SpellRows row{Timestamp{123}, 10, parser};

    try {
	while (true) {
	    Episode episode{row, parser};
	    episode.print(lookup);
	    std::cout << std::endl;
	    row.fetch_next_row();
	}
	    
    } catch (const RowBufferException::NoMoreRows &) {
	std::cout << "Finished fetching all rows" << std::endl;
    }

}
