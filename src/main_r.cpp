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
Rcpp::List make_acs_dataset(const Rcpp::CharacterVector & config_path) {

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

	std::map<std::string, Rcpp::NumericVector> event_counts;
	Rcpp::CharacterVector nhs_numbers;
	Rcpp::NumericVector index_dates;
	Rcpp::LogicalVector pci_triggered;
	Rcpp::NumericVector age_at_index;
	Rcpp::LogicalVector stemi_presentation;
	Rcpp::LogicalVector death_after;
	Rcpp::NumericVector index_to_death;	
	Rcpp::CharacterVector cause_of_death;
	
	unsigned cancel_counter{0};
	unsigned ctrl_c_counter_limit{10};
	const auto all_groups{parser->all_groups(lookup)};
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
						    lookup,
						    print)};

                    record.print(lookup);

                    // Get the counts before and after for this record
		    auto before{record.counts_before()};
		    auto after{record.counts_after()};
		    for (const auto & group : all_groups) {
			event_counts[group.name(lookup) + "_before"].push_back(before[group]);
			event_counts[group.name(lookup) + "_after"].push_back(after[group]);
		    }

		    nhs_numbers.push_back(std::to_string(record.nhs_number()));
		    index_dates.push_back(record.index_date());
		    pci_triggered.push_back(primary_pci(first_episode(index_spell), pci));
		    stemi_presentation.push_back(get_stemi_presentation(index_spell, stemi));

                    try {
			age_at_index.push_back(record.age_at_index().read());
		    } catch (const Integer::Null &) {
			age_at_index.push_back(NA_REAL);		
		    }

		    death_after.push_back(record.death_after());

		    if (record.death_after()) {
			index_to_death.push_back(record.index_to_death().value().value());
		    } else {
                        index_to_death.push_back(NA_REAL);
		    }

		    if (record.death_after()) {
			if (record.cardiac_death()) {
			    cause_of_death.push_back("cardiac");
			} else {
			    cause_of_death.push_back("all_cause");			    
			}
		    } else {
			cause_of_death.push_back("no_death");
		    }
		}
	    
	    } catch (const RowBufferException::NoMoreRows &) {
		std::cout << "Finished fetching all rows" << std::endl;
		break;
	    }
	}

	Rcpp::List table_r;
	table_r["nhs_number"] = nhs_numbers;
	table_r["index_date"] = index_dates;
	table_r["pci_triggered"] = pci_triggered;
	table_r["age_at_index"] = age_at_index;
	table_r["stemi_presentation"] = stemi_presentation;
	table_r["death_after"] = death_after;
	table_r["index_to_death"] = index_to_death;
	table_r["cause_of_death"] = cause_of_death;
	for (const auto & [column_name, counts] : event_counts) {
	    table_r[column_name] = counts;
	}

	return table_r;

    } catch (const std::runtime_error & e) {
	Rcpp::Rcout << "Failed with error: " << e.what() << std::endl;
	return Rcpp::List{};
    }
}
