#include <iostream>
//#include "acs.hpp"
#include "../clinical_code.hpp"
#include "../episode.hpp"
#include "../episode_row.hpp"
#include "../spell_rows.hpp"
#include "../spell.hpp"
#include "../config.hpp"
#include "../sql_query.hpp"
#include "../sql_connection.hpp"
#include "../patient.hpp"

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
    auto config{load_config_file("../../config.yaml")};
    auto parser{new_clinical_code_parser(config["parser"], lookup)};
    auto sql_connection{new_sql_connection(config["connection"])};
    auto sql_query{make_acs_sql_query(config["sql_query"], false, nhs_number)};

    std::cout << sql_query << std::endl;

    ClinicalCodeMetagroup acs{config["code_groups"]["acs"], lookup};
    ClinicalCodeMetagroup pci{config["code_groups"]["pci"], lookup};
    
    auto row{sql_connection.execute_direct(sql_query)};

    while (true) {
	try {
	    Patient patient{row, parser};

	    auto index_spells{get_acs_index_spells(patient.spells(), acs, pci)};

	    if (index_spells.empty()) {
		continue;
	    }
	    
	    std::cout << "Patient = " << patient.nhs_number() << std::endl;
	    for (const auto & spell : index_spells) {

		AcsRecord record{spell};

		std::cout << "INDEX SPELL:" << std::endl;
		spell.print(lookup, 4);

		// Do not add secondary procedures into the counts, because they
		// often represent the current index procedure (not prior procedures)
		for (const auto & group : get_index_secondaries(spell, CodeType::Diagnosis)) {
		    record.push_before(group);
		}

		auto spells_before{get_spells_in_window(patient.spells(), spell, -365*24*60*60)};
		std::cout << "SPELLS BEFORE INDEX:" << std::endl;
                for (const auto & spell_before : spells_before) {
		    spell_before.print(lookup, 4);
		}
		for (const auto & group : get_all_groups(spells_before)) {
		    record.push_before(group);
		}

		auto spells_after{get_spells_in_window(patient.spells(), spell, 365*24*60*60)};
		std::cout << "SPELLS AFTER INDEX:" << std::endl;
                for (const auto & spell_after : spells_after) {
		    spell_after.print(lookup, 4);
		}
		for (const auto & group : get_all_groups(spells_after)) {
		    record.push_after(group);
		}
		
		std::cout << "INDEX RECORD:" << std::endl;
		record.print(lookup);
		std::cout << std::endl;
		
	    }
	    
	} catch (const RowBufferException::NoMoreRows &) {
	    std::cout << "Finished fetching all rows" << std::endl;
	    break;
	}
    }
}
