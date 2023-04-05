#include "string_lookup.hpp"
#include "config.hpp"
#include "clinical_code.hpp"
#include "acs.hpp"
#include "sql_query.hpp"
#include "patient.hpp"

#include <Rcpp.h>

#include <optional>

// [[Rcpp::export]]
void make_acs_dataset(const Rcpp::CharacterVector & config_path) {

    std::string config_path_str{Rcpp::as<std::string>(config_path)};
    
    try {
	auto lookup{new_string_lookup()};
	auto config{load_config_file(config_path_str)};
        auto parser{new_clinical_code_parser(config["parser"], lookup)};
	auto sql_connection{new_sql_connection(config["connection"])};
	auto sql_query{make_acs_sql_query(config["sql_query"], false, std::nullopt)};

	std::cout << sql_query << std::endl;

	ClinicalCodeMetagroup acs{config["code_groups"]["acs"], lookup};
	ClinicalCodeMetagroup pci{config["code_groups"]["pci"], lookup};
    
	auto row{sql_connection.execute_direct(sql_query)};
    
	while (true) {
	    try {
		Patient patient{row, parser};

		/// Is index event if there is a primary ACS or PCI
		/// in the _first_ episode of the spell
		auto is_acs_index_spell{[&](const Spell &spell) {
		    auto & episodes{spell.episodes()};
		    if (episodes.empty()) {
			return false;
		    }
		    auto & first_episode{episodes[0]};
		    auto primary_acs{acs.contains(first_episode.primary_diagnosis())};
		    auto primary_pci{pci.contains(first_episode.primary_procedure())};
		    return primary_acs or primary_pci;
		}};
	    
		auto index_spells { patient.spells() | std::views::filter(is_acs_index_spell) };

		std::cout << "Patient = " << patient.nhs_number() << std::endl;
		for (const auto & spell : index_spells) {
		    spell.print(lookup, 4);
		}
	    
	    } catch (const RowBufferException::NoMoreRows &) {
		std::cout << "Finished fetching all rows" << std::endl;
		break;
	    }
	}


    } catch (const std::runtime_error & e) {
	Rcpp::Rcout << "Failed with error: " << e.what() << std::endl;
    }

    
}
