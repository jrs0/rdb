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

void print_patient_episodes(const std::string & nhs_number) {
    auto lookup{new_string_lookup()};
    auto config{load_config_file("../../config.yaml")};
    auto parser{new_clinical_code_parser(config["parser"], lookup)};
    auto sql_connection{new_sql_connection(config["connection"])};
    auto sql_query{make_acs_sql_query(config["sql_query"], false, nhs_number)};

    std::cout << sql_query << std::endl;

    try {
	auto row{sql_connection.execute_direct(sql_query)};
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

int main(int argc, char ** argv) {

    if (argc < 2) {
	std::cout << "Must pass nhs_number" << std::endl;
	return 1;
    }

    std::string nhs_number{argv[1]};
    print_patient_episodes(nhs_number);
     
}
