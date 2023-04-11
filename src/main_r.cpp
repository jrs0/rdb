#include "string_lookup.hpp"
#include "config.hpp"
#include "clinical_code.hpp"
#include "sql_query.hpp"
#include "sql_connection.hpp"
#include "patient.hpp"

#include "acs.hpp"

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

	std::vector<AcsRecord> acs_records;
	
	auto print{config["print"].as<bool>()};

	std::cout << "Started fetching rows" << std::endl;

	unsigned cancel_counter{0};
	while (true) {

	    if (++cancel_counter > 10) {
		cancel_counter = 0;
		Rcpp::checkUserInterrupt();
	    }

	    try {
		
		Patient patient{row, parser};

		auto index_spells{get_acs_index_spells(patient.spells(), acs, pci)};

		if (index_spells.empty()) {
		    continue;
		}

		if (print) {
		    std::cout << "Patient = " << patient.nhs_number()
			      << std::endl;
		}
		for (const auto & index_spell : index_spells) {
		    auto record{get_record_from_index_spell(patient,
							    index_spell,
							    lookup,
							    print)};
		    acs_records.push_back(record);
		}
	    
	    } catch (const RowBufferException::NoMoreRows &) {
		std::cout << "Finished fetching all rows" << std::endl;
		break;
	    }
	}

	std::cout << "Total records: " << acs_records.size() << std::endl;

    } catch (const std::runtime_error & e) {
	Rcpp::Rcout << "Failed with error: " << e.what() << std::endl;
    }
}
