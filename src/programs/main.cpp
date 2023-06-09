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
#include "patient.h"

#include "../cmdline.hpp"
#include "../acs.hpp"

void sort_spells_by_date(std::vector<Spell> & spells) {
    std::ranges::sort(spells, [](const auto &a, const auto &b) {
	return a.start_date() < b.start_date();
    });
}

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

    ClinicalCodeMetagroup acs{config["code_groups"]["acs"], lookup};
    ClinicalCodeMetagroup pci{config["code_groups"]["pci"], lookup};
    
    auto row{sql_connection.execute_direct(sql_query)};

    std::vector<AcsRecord> acs_records;

    auto print{config["print"].as<bool>()};

    std::cout << "Started fetching rows" << std::endl;
    
    while (true) {
	try {
	    Patient patient{row, parser};

	    auto index_spells{get_acs_index_spells(patient.spells(), acs, pci)};

	    if (index_spells.empty()) {
		continue;
	    }

	    if (print) {
		std::cout << "Patient = " << patient.nhs_number() << std::endl;
	    }
	    for (const auto & index_spell : index_spells) {
		auto record{get_record_from_index_spell(patient, index_spell, lookup, print)};
		acs_records.push_back(record);
	    }
	    
	} catch (const RowBufferException::NoMoreRows &) {
	    std::cout << "Finished fetching all rows" << std::endl;
	    break;
	}
    }

    std::cout << "Total records: " << acs_records.size() << std::endl;
}
