#include <iostream>
//#include "acs.hpp"
#include "clinical_code.h"
#include "episode.h"
#include "episode_row.h"
#include "spell_rows.h"
#include "spell.h"
#include "config.h"
#include "sql_query.h"
#include "sql_connection.h"

#include "cmdline/cmdline.hpp"

int main(int argc, char ** argv) {

    CommandLine cmd;
    
    const std::string program_name{ "spells" };
    const std::string version{ "v0.1.0" };
    const std::string short_desc{"A program for getting patient spells"};
    const std::string long_desc{R"xyz(spells is a program for reading a HES table and returning readable information about patient episodes and spells.)xyz"};
    
    cmd.addOption<std::string>('n', "nhs-number",
			       "The pseudo-NHS number of the patient to search");

    if(cmd.parse(argc, argv) != 0) {
	std::cerr << "An error occurred while parsing the command line arguments"
		  << std::endl;
	return 1;
    }

    auto nhs_number{cmd.get<std::string>('n')};

    auto lookup{new_string_lookup()};
    auto config{load_config_file("../../scripts/config.yaml")};
    auto parser{new_clinical_code_parser(config["parser"], lookup)};
    auto sql_connection{new_sql_connection(config["connection"])};
    auto sql_query{make_acs_sql_query(config["sql_query"], false, nhs_number)};

    std::cout << sql_query << std::endl;

    auto row{sql_connection.execute_direct(sql_query)};
    std::vector<Spell> spells;
    
    int count{0}; 
    while (true) {
	try {
	    spells.push_back(Spell{row, parser});
	} catch (const RowBufferException::NoMoreRows &) {
	    std::cout << "Finished fetching all rows" << std::endl;
	    break;
	}
    count++;
    }



    struct {
	bool operator()(const Spell & a, const Spell & b) const {
	    return a.start_date() < b.start_date();
	}
    } by_start_date;

    std::ranges::sort(spells, by_start_date);

    // for (const auto & spell : spells) {
	//     spell.print(std::cout, lookup);
	//     std::cout << std::endl;
    // }
    std::cout << "Done: count = " << count << std::endl; 
}
