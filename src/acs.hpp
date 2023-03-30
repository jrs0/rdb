// This file interprets hospital episode data into records
// containing an index event, and counts of certain events
// that occur before and after the index event.
//
// For the purposes of this file, an episode is only of interest
// if its diagnosis or procedure falls within a group of
// interest. If it does not, then that episode is ignored.
//
// Every class whose job it is to parse a block of (ordered)
// rows does it in the same way: the first row is assumed to
// point to the first row of the block (which is used to identify
// the block). The class then steps through rows until it reaches
// one that is outside its current block. At this point it
// returns, and the next block is handled by another class
// (which can assume its first row is ready in the buffer).
// This approach avoids the need to know up front how long
// each block is going to be. It does however require a column
// which uniquely distinguishes the blocks from each other.

#ifndef ACS_HPP
#define ACS_HPP

#include <sstream>
#include <optional>
#include "yaml.hpp"
#include "category.hpp"
#include "sql_connection.hpp"
#include "sql_types.hpp"
#include "row_buffer.hpp"
#include "mem_row_buffer.hpp"
#include "patient.hpp"
#include "set_utils.hpp"

/// Index episode are those that contain
/// an ACS or a PCI diagnosis. There is at most
/// one index event from every spell -- this is
/// supposed to avoid double counting (especially)
/// diagnoses from the same spell, that may
/// correspond to the same underlying event.
///
/// The start date of the index episode is used as
/// the index date. A flag records whether the triggering
/// event was a diagnosis or a procedure. The patient
/// age at that episode is recorded.
class IndexEvent {
public:

    /// Thrown if not an index event
    struct NotIndexEvent {};

    /// Thrown if the event is an index event, but
    /// the date is not known (and therefore cannot be
    /// used to find previous and subsequent events).
    struct NoEpisodeDate {};
    
    /// The information for the index event comes from
    /// a single episode. This constructor throws an
    /// exception if the episode is not an index event.
    /// This keeps the logic for what is an index event
    /// contained inside this constructor.
    IndexEvent(const Episode & episode,
	       const std::set<std::string> & index_diagnoses,
	       const std::set<std::string> & index_procedures,
	       const std::string & stemi_flag) {

	// To store the intersections between the episode
	// diagnoses/procedures and the relevant index list
	// in the config file
	auto relevant_diagnoses{intersection(index_diagnoses,
					     episode.diagnoses())};
	auto relevant_procedures{intersection(index_procedures,
					      episode.procedures())};

	// Check whether the there are any relevant events
	// in this episode
	if (relevant_diagnoses.empty() and relevant_procedures.empty()) {
	    throw NotIndexEvent{};
	}

	// The other diagnosis and procedures are the ones present in the
	// episode but which are not the triggering ones -- they are
	// supposed to be a proxy for underlying conditions (TODO fix this
	// logic)
	other_diagnoses_ = set_difference(episode.diagnoses(),
					  relevant_diagnoses);
	other_procedures_ = set_difference(episode.procedures(),
					   relevant_procedures);
        
	// Record stemi presentation
	stemi_presentation_ = episode.diagnoses().contains(stemi_flag);
	
	// Check if the index event is triggered by a diagnosis
	// of procedure. Procedures take priority here.
	procedure_triggered_ = (relevant_procedures.size() > 0);
	
	// Store the age recorded in the episode
	try {
	    age_at_index_ = episode.age_at_episode().read();
	} catch (const NullValue &) {
	    // Leave empty value
	}

	// Store the episode start date as the index event date
	date_ = episode.episode_start();
	if (date_.null()) {
	    throw NoEpisodeDate{};
	}
    }

    auto age_at_index() const {
	return age_at_index_;
    }
    
    Timestamp date() const {
	return date_;
    }

    const auto & other_diagnoses() const {
	return other_diagnoses_;
    }

    const auto & other_procedures() const {
	return other_procedures_;
    }

    std::string type() const {
	if (procedure_triggered_) {
	    return "procedure";
	} else {
	    return "diagnosis";
	}
    }

    auto stemi_presentation() const {
	return stemi_presentation_;
    }
    
    /// Print the index event
    void print() const {
	std::cout << "Index: ";
	date_.print();
	std::cout << " Procedure? " << procedure_triggered_;
	std::cout << "; Age: " << age_at_index_.value_or(-1);
    }
private:

    // List of other diagnoses and procedures, not involved in
    // triggering inclusion as an index event, that will be taken
    // as prior conditions
    std::set<std::string> other_procedures_;
    std::set<std::string> other_diagnoses_;
    
    /// True if a procedure generated the index event, false otherwise
    bool procedure_triggered_;
    bool stemi_presentation_;
    Timestamp date_;
    std::optional<std::size_t> age_at_index_; ///< Age at the index event
};

/// Print a map of key values. Both T and V need << overloads
template<typename K, typename V>
void print_map(const std::map<K, V> & map, std::size_t pad = 0) {
    for (const auto & [key, value] : map) {
	std::cout << std::string(pad, ' ')
		  << key << ": " << value
		  << std::endl; 
    }
}

/// An index event along with events before and
/// after.
class Record {
public:

    /// Pass a patient, containing a set of spells to search, and
    /// an index event, defining the base date for the search.
    Record(const Patient & patient,
	   const IndexEvent & index_event)
	: nhs_number_{patient.nhs_number()}, index_event_{index_event} {
	
	// Get the date of the index event
	Timestamp base_date{index_event_.date()};

	// Make the start and end dates within which to search
	auto start_date{base_date + -356*24*60*60};
	auto end_date{base_date + 356*24*60*60};

	// Incorporate all the other diagnoses/procedures (not
	// involved in triggering the index event) into the
	// counts for previous conditions. For diagnoses, this logic
	// is OK, especially if they are secondary diagnoses. This is
	// most important for index events coming from the first episode
	// in a spell (otherwise it is likely that patient history
	// would be picked up in a previous episode in the loops below).
	// There is a flag in the database for whether the diagnosis was
	// present on admission -- it would be much better to use this,
	// ion conjunction with primary/secondary diagnosis logic.
	for (const auto & diagnosis : index_event.other_diagnoses()) {
	    num_events_before_[diagnosis]++;
	}
	for (const auto & procedure : index_event.other_procedures()) {
	    num_events_before_[procedure]++;
	}
	
	// Check if death occured in the after window, and if
	// so, add the cause of death 

	// Loop over all the spells in the patient
	for (const auto & spell : patient.spells()) {

	    // Loop over all episodes in the spell
	    for (const auto & episode : spell.episodes()) {

		auto date{episode.episode_start()};
		
		// Loop at spells before. Note that the strict inequality
		// will also reject the index episode
		if ((date < base_date) and (date > start_date)) {
		    // Accumulate all the diagnosis and procedure
		    // codes from this episode into the count for
		    // before
		    for (const auto & diagnosis : episode.diagnoses()) {
			num_events_before_[diagnosis]++;
		    }
		    for (const auto & procedure : episode.procedures()) {
			num_events_before_[procedure]++;
		    }
		}

		// Look at spells after
		if ((date > base_date) and (date < end_date)) {
		    // Accumulate all the diagnosis and procedure
		    // codes from this episode into the count for
		    // after
		    for (const auto & diagnosis : episode.diagnoses()) {
			num_events_after_[diagnosis]++;
		    }
		    for (const auto & procedure : episode.procedures()) {
			num_events_after_[procedure]++;
		    }
		}
	    }
	}
    }

    const auto & counts_before() const {
	return num_events_before_;
    }

    const auto & counts_after() const {
	return num_events_after_;
    }

    auto nhs_number() const {
	return nhs_number_;
    }
    
    auto index_date() const {
	return index_event_.date().read();
    }

    auto index_type() const {
	return index_event_.type();
    }

    auto age_at_index() const {
	return index_event_.age_at_index();
    }

    auto stemi_presentation() const {
	return index_event_.stemi_presentation();
    }

    void print() const {
	std::cout << "Record: Patient " << nhs_number_
		  << ""
		  << std::endl << " ";
	index_event_.print();
	std::cout << std::endl;
	std::cout << " Count before:" << std::endl;
	print_map(num_events_before_, 3);
	std::cout << " Count after:" << std::endl;
	print_map(num_events_after_, 3);
    }
    
private:
    long long unsigned nhs_number_;
    IndexEvent index_event_;
    /// A map from a procedure or diagnosis group to the
    /// number of that event that occured in the window before
    /// or after the event.
    std::map<std::string, std::size_t> num_events_before_;
    std::map<std::string, std::size_t> num_events_after_;
};

// Quick benchmarks:
//
// For 500,000 rows:
//
// Without optimisations:
// - Query takes 24 seconds
// - Row fetching/processing takes 38 seconds
//
// With optimisations:
// - Query takes 26 seconds
// - Row fetching/processing takes 13 seconds
//
// Printing does not add any overhead.
// 
// The query fetch time is a function of the number
// of rows, but not the optimisation level because
// it happens on the server side.
//
// Projected processing time for 10,000,000 rows
// is 25 + 20 * 13 = 285 seconds. Tested on full
// query -- takes 288 seconds.
//
// For interest, prior to fixing the tree-copying
// bug, for 50,000 rows, optimisations enabled:
// - Query takes 24 seconds
// - Row fetching/processing takes 35 seconds
//
// So the speedup is (35/50000) / (13/500000)
// = 25 times.
//
//
// After adding in all the diagnosis/procedure
// columns (for 50,000 rows, with optimisations)
// - Query takes 34 seconds
// - Row fetching/processing takes 21 seconds
//
// After short-circuiting the diagnosis and procedure
// columns:
// - Query takes 34 seconds
// - Row fetching/processing takes 26 seconds.
//
//
//

std::string make_acs_sql_query(const YAML::Node & config, bool with_mortality = true) {

    std::stringstream query;

    query << "select ";

    if (config["result_limit"]) {
	std::size_t result_limit{config["result_limit"].as<std::size_t>()};
	std::cout << "Using result limit " << result_limit << std::endl;
	query << "top " << result_limit << " ";
    }
    
    query << "episodes.* ";
    if (with_mortality) {
	query << ", mort.REG_DATE_OF_DEATH as date_of_death,"
	      << ", mort.S_UNDERLYING_COD_ICD10 as cause_of_death,"
	      << ", mort.Dec_Age_At_Death as age_at_death ";
    }
    query << "from (select " 
	  << "AIMTC_Pseudo_NHS as nhs_number,"
	  << "AIMTC_Age as age_at_episode,"
	  << "PBRspellID as spell_id,"
	  << "StartDate_ConsultantEpisode as episode_start,"
	  << "EndDate_ConsultantEpisode as episode_end,"
	  << "AIMTC_ProviderSpell_Start_Date as spell_start,"
	  << "AIMTC_ProviderSpell_End_Date as spell_end ";

    // Add all the diagnosis columns
    auto diagnoses{expect_string_vector(config["parser_config"]["diagnoses"], "source_columns")};
    for (const auto & diagnosis : diagnoses) {
	query <<  "," << diagnosis;
    }
    auto procedures{expect_string_vector(config["parser_config"]["procedures"], "source_columns")};
    for (const auto & procedure : procedures) {
	query << "," << procedure;
    }

    query << " from abi.dbo.vw_apc_sem_001 "
	  << "where datalength(AIMTC_Pseudo_NHS) > 0 "
	  << "and datalength(pbrspellid) > 0 "
	  << ") as episodes ";
    if (with_mortality) {
	query << "left join abi.civil_registration.mortality as mort "
	      << "on episodes.nhs_number = mort.derived_pseudo_nhs ";
    }
    query << "order by nhs_number, spell_id; ";
    
    return query.str();
}

SQLConnection make_sql_connection(const YAML::Node & config) {
    // Connect to DSN or with credential file
    if (config["data_sources"]["connection"]["dsn"]) {
	auto dsn{config["data_sources"]["connection"]["dsn"].as<std::string>()};
	std::cout << "Connection to DSN " << dsn << std::endl;
	return SQLConnection{dsn};
    } else if (config["data_sources"]["connection"]["cred"]) {
	auto cred_path{config["data_sources"]["connection"]["cred"].as<std::string>()};
	auto cred{YAML::LoadFile(cred_path)};
        return SQLConnection{cred};
    } else {
	throw std::runtime_error("You need either dsn or cred in the connection "
				 "block for data_sources");
    }

}

std::vector<Record> get_acs_records(const YAML::Node & config) {
        
    std::vector<Record> records;

    // Contains the parsers for OPCS and ICD codes
    CodeParser code_parser{config["parser_config"]};

    // Load the index diagnosis and procedures lists
    auto include{config["index_event"]["include"]};
    auto index_diagnoses{expect_string_set(include, "diagnoses")};
    auto index_procedures{expect_string_set(include, "procedures")};
    auto stemi_flag{config["index_event"]["include"]["stemi_flag"].as<std::string>()};
    
    auto con{make_sql_connection(config)};

    // Whether to join the mortality table or not
    bool with_mortality{false};
	
    std::cout << "Executing statement" << std::endl;
    auto query{make_acs_sql_query(config, false)};
    std::cout << std::endl << "Query: " << query << std::endl << std::endl;
    auto row{con.execute_direct(query)};
        
    std::cout << "Starting to fetch rows" << std::endl;

    // sql statement that fetches all episodes for all patients
    // ordered by nhs number, then spell id.
	
    // Make the first fetch
    row.fetch_next_row();

    // Count the patients
    std::size_t patient_count{0};
    
    while (true) {
	try {
	    /// Parse a block of rows into a Patient,
	    /// which contains a list of Spells, each of
	    /// which contains a list of episodes.
	    Patient patient{row, code_parser};
	    patient_count++;

	    if (not patient.empty()) {
		
		// Loop over all the spells for this patient
		for (const auto & spell : patient.spells()) {

		    // Loop over all the episodes in this spell
		    for (const auto & episode : spell.episodes()) {

			try {
			    // Test if this episode is an index event
			    IndexEvent index_event{episode,
						   index_diagnoses,
						   index_procedures,
						   stemi_flag};
			    // For this index event, construct the
			    // record by looking at at all episodes
			    // before/after the index event for this
			    // patient

			    Record record{patient, index_event};
			    std::cout << "Up to row "
				      << row.current_row_number()
				      << std::endl;
			    record.print();
			    records.push_back(record);
			    
			    // Do not consider any episodes from this
			    // spell as index event.
			    break;
				
			} catch (const IndexEvent::NotIndexEvent &) {
			    // Continue
			    //std::cout << "Not index event" << std::endl;
			} catch (const IndexEvent::NoEpisodeDate &) {
			    // Continue
			    std::cout << "No Episode date" << std::endl;
			}
		    }
		}
	    }
	} catch (const std::logic_error & e) {
	    // There are no more rows
	    std::cout << "No more rows -- finished" << std::endl;
	    std::cout << e.what() << std::endl;
	    break;
	}
    }
    std::cout << "Total patients = " << patient_count << std::endl;
    std::cout << "Number of index event = " << records.size() << std::endl;
    std::cout << "Traversed " << row.current_row_number() << " rows"
	      << std::endl;
    
    return records;
}

#endif
