#include "string_lookup.hpp"
#include "config.hpp"
#include "clinical_code.hpp"
#include "sql_query.hpp"
#include "sql_connection.hpp"
#include "patient.hpp"

#include "acs.hpp"

#include <Rcpp.h>
#include "r_factor.hpp"

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

	ClinicalCodeMetagroup acs_metagroup{config["code_groups"]["acs"], lookup};
	ClinicalCodeMetagroup pci_metagroup{config["code_groups"]["pci"], lookup};
	ClinicalCodeMetagroup cardiac_death_metagroup{config["code_groups"]["cardiac_death"], lookup};
	ClinicalCodeMetagroup stemi_metagroup{config["code_groups"]["stemi"], lookup};

	std::cout << "Executing query" << std::endl;
        auto row{sql_connection.execute_direct(sql_query)};

        auto print{config["print"].as<bool>()};

	std::cout << "Started fetching rows" << std::endl;

	std::map<std::string, Rcpp::NumericVector> event_counts;
	RFactor nhs_numbers;
	Rcpp::NumericVector index_dates;
	RFactor index_type;
	Rcpp::NumericVector age_at_index;
	RFactor stemi_presentation;
        Rcpp::NumericVector index_to_death;	
	RFactor cause_of_death;
	
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
		auto index_spells{get_acs_index_spells(patient.spells(), acs_metagroup, pci_metagroup)};
		if (index_spells.empty()) {
		    continue;
		}

		auto nhs_number{patient.nhs_number()};
		const auto & mortality{patient.mortality()};

		for (const auto & index_spell : index_spells) {

		    if (index_spell.empty()) {
			continue;
		    }

		    const auto & first_episode{::first_episode(index_spell)};

                    auto age_at_index{first_episode.age_at_episode()};
		    auto date_of_index{first_episode.episode_start()};		    
		    auto death_after{false};
		    auto cardiac_death{false};
		    std::optional<TimestampOffset> index_to_death;		    
		    
		    auto stemi_flag{get_stemi_presentation(index_spell, stemi_metagroup)};
		    
		    EventCounter event_counter;
    
		    // Do not add secondary procedures into the counts, because they
		    // often represent the current index procedure (not prior procedures)
		    for (const auto & group : get_index_secondaries(index_spell, CodeType::Diagnosis)) {
			event_counter.push_before(group);
		    }
    
		    auto spells_before{get_spells_in_window(patient.spells(), index_spell, -365*24*60*60)};
    
		    for (const auto & group : get_all_groups(spells_before)) {
			event_counter.push_before(group);
		    }

		    auto spells_after{get_spells_in_window(patient.spells(), index_spell, 365*24*60*60)};
		    for (const auto & group : get_all_groups(spells_after)) {
			event_counter.push_after(group);
		    }
		    
		    if (not mortality.alive()) {

			auto date_of_death{mortality.date_of_death()};
			if (not date_of_death.null() and not date_of_index.null()) {

			    if (date_of_death < date_of_index) {
				throw std::runtime_error("Unexpected date of death before index date at patient"
							 + std::to_string(nhs_number));
			    }

			    // Check if death occurs in window after (hardcoded for now)
			    index_to_death = date_of_death - date_of_index};
			    if (index_to_death.value() < years(1)) {
				death_after = true;
				auto cause_of_death{mortality.cause_of_death()};
				if (cause_of_death.has_value()) {
				    cardiac_death = cardiac_death_metagroup.contains(cause_of_death.value());
				}
			    }
			}
		    }

                    // Get the counts before and after for this record
		    auto before{event_counter.counts_before()};
		    auto after{event_counter.counts_after()};
		    for (const auto & group : all_groups) {
			event_counts[group.name(lookup) + "_before"].push_back(before[group]);
			event_counts[group.name(lookup) + "_after"].push_back(after[group]);
		    }

		    nhs_numbers.push_back(std::to_string(nhs_number));
		    index_dates.push_back(date_of_index);

		    auto pci_triggered{primary_pci(first_episode(index_spell), pci_metagroup)};
		    if (pci_triggered) {
			index_type.push_back("PCI");
		    } else {
			index_type.push_back("ACS");			
		    }

		    if (stemi_flag) {
			stemi_presentation.push_back("STEMI");
		    } else {
			stemi_presentation.push_back("NSTEMI");
		    }
		    
                    try {
			age_at_index.push_back(age_at_index.read());
		    } catch (const Integer::Null &) {
			age_at_index.push_back(NA_REAL);		
		    }

		    if (death_after) {
			index_to_death.push_back(index_to_death.value().value());
			if (cardiac_death) {
			    cause_of_death.push_back("cardiac");
			} else {
			    cause_of_death.push_back("all_cause");			    
			}
		    } else {
                        index_to_death.push_back(NA_REAL);
			cause_of_death.push_back("no_death");
		    }
		}
	    
	    } catch (const RowBufferException::NoMoreRows &) {
		std::cout << "Finished fetching all rows" << std::endl;
		break;
	    }
	}

	Rcpp::List table_r;
	table_r["nhs_number"] = nhs_numbers.get();
	table_r["index_date"] = index_dates;
	table_r["index_type"] = index_type.get();
	table_r["age_at_index"] = age_at_index;
	table_r["stemi_presentation"] = stemi_presentation.get();
	table_r["index_to_death"] = index_to_death;
	table_r["cause_of_death"] = cause_of_death.get();
	for (const auto & [column_name, counts] : event_counts) {
	    table_r[column_name] = counts;
	}

	return table_r;

    } catch (const std::runtime_error & e) {
	Rcpp::Rcout << "Failed with error: " << e.what() << std::endl;
	return Rcpp::List{};
    }
}
