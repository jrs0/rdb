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
	auto sql_query{make_acs_sql_query(config["sql_query"], true, std::nullopt)};

	std::cout << sql_query << std::endl;
	
	ClinicalCodeMetagroup acs{config["code_groups"]["acs"], lookup};
	ClinicalCodeMetagroup pci{config["code_groups"]["pci"], lookup};
	ClinicalCodeMetagroup cardiac_death{config["code_groups"]["cardiac_death"], lookup};
	ClinicalCodeMetagroup stemi{config["code_groups"]["cardiac_death"], lookup};
	
	auto row{sql_connection.execute_direct(sql_query)};

	//std::vector<AcsRecord> acs_records;

        auto print{config["print"].as<bool>()};

	std::cout << "Started fetching rows" << std::endl;

	auto all_groups{parser->all_groups()};
	
	// Make a table to store the counts.
	std::map<std::string, Rcpp::NumericVector> event_counts;

	// Make a vector to store the nhs numbers. Convert back
	// to characters so there are no overflow issues with 64
	// bit ints in R
	Rcpp::CharacterVector nhs_numbers;

	// Make a vector of timestamps for the date of each index
	// event
	Rcpp::NumericVector index_dates;

	// The event triggering inclusion as an index event
	// (ACS or PCI)
	Rcpp::CharacterVector index_type;

	// The event triggering inclusion as an index event
	Rcpp::NumericVector age_at_index;
	
	// The event triggering inclusion as an index event
	Rcpp::LogicalVector stemi_presentation;

	// Whether death occured in the "after" period
	Rcpp::LogicalVector death_after;

	// Time between index event and death (if death_after)
	Rcpp::LogicalVector index_to_death;	

	// Cause of death (if it occurred)
	Rcpp::CharacterVector cause_of_death;
	
	unsigned cancel_counter{0};
	unsigned ctrl_c_counter_limit{10};
	while (true) {

	    if (++cancel_counter > ctrl_c_counter_limit) {
		Rcpp::checkUserInterrupt();
		cancel_counter = 0;
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
		    auto mortality{patient.mortality()};
		    mortality.print(lookup);
		}
		for (const auto & index_spell : index_spells) {
		    auto record{
			get_record_from_index_spell(patient, index_spell,
						    cardiac_death,
						    stemi,
						    lookup,
						    print)};



                    record.print(lookup);

                    // Get the counts before and after for this record
		    auto before{record.counts_before()};
		    auto after{record.counts_after()};
	    
		    // Add all the counts columns
		    for (const auto & group : all_groups) {
			auto group_name{group.name()};
			event_counts[group_name + "_before"].push_back(before[group]);
			event_counts[group_name + "_after"].push_back(after[group]);
		    }
		    
		    // Get the nhs number
		    nhs_numbers.push_back(std::to_string(record.nhs_number()));

		    // Get the index event date
		    index_dates.push_back(record.index_date());

		    // Get the index event type
		    index_type.push_back(record.index_type());

		    // Get the stemi presentation flag
		    stemi_presentation.push_back(record.stemi_presentation());

		    // Age at the index event episode
		    try {
			age_at_index.push_back(record.age_at_index().value());
		    } catch (const std::bad_optional_access &) {
			age_at_index.push_back(NA_REAL);		
		    }


		    
		    nhs_numbers.push_back()
		    
                    //acs_records.push_back(record);
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
