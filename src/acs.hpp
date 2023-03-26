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

#include <optional>
#include "yaml.hpp"
#include "category.hpp"
#include "sql_connection.hpp"
#include "sql_types.hpp"
#include "row_buffer.hpp"

template<typename T>
std::set<T> intersection(const std::set<T> s1, const std::set<T> s2) {
    std::set<T> out;
    std::ranges::set_intersection(s1, s2, std::inserter(out, out.begin()));
    return out;
}

/// Get the vector of source columns from the config file node
std::vector<std::string> source_columns(const YAML::Node & config) {
    return config["source_columns"].as<std::vector<std::string>>();
}

/// Helper function to read a set of columns of codes, parse them using
/// a specified parser, and collect the valid ones into a flat vector
/// (in the same order as the column order). A code is considered invalid
/// (and not included) if the parser throws a runtime error during
/// parsing. Look at the TopLevelcategory to see what is covered.
std::set<std::string>
merge_groups_from_columns(const std::vector<std::string> & columns,
			  const RowBuffer auto & row,
			  TopLevelCategory & parser) {
    std::set<std::string> result;
    for (const auto & column_name : columns) {
	try {
	    auto raw_code{column<Varchar>(column_name, row).read()};
	    auto groups{parser.code_groups(raw_code)};
	    result.insert(groups.begin(), groups.end());
	} catch (const NullValue & /* NULL in column */) {
	    // Continue
	} catch (const std::runtime_error & /* invalid or not found */) {
	    // Continue
	} catch (const std::out_of_range & e) {
	    throw std::runtime_error("Could not read from column "
				     + column_name);
	}
    }
    return result;
}

/// This class has two jobs -- keep the diagnoses and
/// procedure parsers close together; and map collections
/// of groups as defined in the codes file into "meta" groups
/// that will become the column names in the data frame.
///
/// This class owns its parser, so it will be thread safe
/// at the cost of duplicated copies of the codes tree
class CodeParser {
public:
    CodeParser(const YAML::Node & parser_config)
	: include_all_{parser_config["all"].as<bool>()},
	  procedures_{load_codes_helper(parser_config["procedures"])},
	  diagnoses_{load_codes_helper(parser_config["diagnoses"])},
	  procedure_columns_{source_columns(parser_config["procedures"])},
	  diagnosis_columns_{source_columns(parser_config["diagnoses"])}
    {
	// Open both the codes files and make the
	// categories -- also store maps that group
	// together categories into higher level groups,
	// as defined by the parser_config
    }

    /// Returns the cause of death as a group, "_all_cause"
    /// if the code is valid but is not in a group, or "_unknown"
    /// if the code fails to parse
    std::set<std::string> cause_of_death(const RowBuffer auto & row) {
	try {
	    auto raw_code{column<Varchar>("cause_of_death", row).read()};
	    // Note that the cause of death is an ICD field
	    auto groups{diagnoses_.code_groups(raw_code)};
	    if (groups.size() == 0) {
		// In this case, the code was valid but not in
		// any group
		groups.insert("_all_cause");
	    }
	    return groups;
	} catch (const NullValue & /* SQL NULL value */) {	    
	    return {"_unknown"};
	} catch (const std::runtime_error & /* invalid or not found */) {
	    return {"_unknown"};
	} catch (const std::out_of_range & e) {
	    throw std::runtime_error("Could not read from 'cause of death'");
	}
    }
    
    /// Parse all the procedure codes into a flat list, omitting
    /// any codes that are invalid.  If the
    /// "all" key is enabled, then all codes are placed into a group called
    /// "_all", which will be included in the output in addition to the other
    /// groups.
    std::set<std::string> all_procedures(const RowBuffer auto & row) {
	auto all_groups{merge_groups_from_columns(procedure_columns_,
						  row, procedures_)};
	if (include_all_) {
	    all_groups.insert("_all");
	}
	return all_groups;
    }
    
    /// Parse all the diagnosis codes
    std::set<std::string> all_diagnoses(const RowBuffer auto & row) {
	auto all_groups{merge_groups_from_columns(diagnosis_columns_,
						  row, diagnoses_)};
	if (include_all_) {
	    all_groups.insert("_all");
	}
	return all_groups;
    }
    
private:
    
    /// Whether to include _all group
    bool include_all_;

    TopLevelCategory procedures_;
    TopLevelCategory diagnoses_;

    std::vector<std::string> procedure_columns_;
    std::vector<std::string> diagnosis_columns_;

    //std::map<std::string, std::string> procedures_group_map_;
    //std::map<std::string, std::string> diagnoses_group_map_;
};

/// The purpose of the Episode class is to parse all the
/// primary and secondary diagnosis and procedure fields
/// and expose them as flat lists of groups (defined by
/// the ICD and OPCS code group files). Codes are only
/// included by group, as defined in the groups file. Codes
/// outside any group are not included in the diagnoses and
/// procedures of this episode. Note that all codes can be
/// included in a catch all "_all" group by enabling the "all"
/// key in the codes file.
class Episode {
public:

    /// For consistency with the other functions, this constructor
    /// will also fetch the next row after it is done (modifying
    /// the row argument)
    Episode(RowBuffer auto & row, CodeParser & code_parser) {
	// Expect a single row as argument, which will be the
	// episode. The episode may contain either a procedure
	// or a diagnosis (including secondaries), or both. The
	// episode relates to what happened under one attending
	// consultant.

	// Need to parse all the primary and secondary fields
	// here using the codes files, so there needs to be
	// a reference to a top_level_category for ICD and OPCS,
	// and also the config file for further grouping. This
	// can probably be abstracted into a wrapper which
	// handles the parsing and mapping

	// Currently just pushes the raw code, need to parse it
	// which also means having the CodeParser around here.
	// This episode should also throw an exception if the
	// episode is not of interest (it is then not included
	// in the spell).

	episode_start_ = column<Timestamp>("episode_start", row);
	episode_end_ = column<Timestamp>("episode_end", row);
	
	procedures_ = code_parser.all_procedures(row);
	diagnoses_ = code_parser.all_diagnoses(row);

	age_at_episode_ = column<Integer>("age_at_episode", row);
	
	row.fetch_next_row();
    }

    std::set<std::string> procedures() const {
	return procedures_;
    }

    std::set<std::string> diagnoses() const {
	return diagnoses_;
    }

    Integer age_at_episode() const {
	return age_at_episode_;
    }

    Timestamp episode_start() const {
	return episode_start_;
    }
    
    /// Returns true if there are no diagnosis or procedure
    /// groups associated with this episode.
    bool empty() const {
	return procedures_.empty() and diagnoses_.empty();
    }
    
    void print() const {
	std::cout << "  Episode: ";
	episode_start_.print();
	std::cout << " - ";
	episode_end_.print();
	std::cout << std::endl;
	std::cout << "    D(";
	for (const auto & diagnosis : diagnoses_) {
	    std::cout << diagnosis << ",";
	}
	std::cout << ") P(";
	for (const auto & procedure : procedures_) {
	    std::cout << procedure << ",";
	}
	std::cout << ")" << std::endl;;
    }
    
private:
    Timestamp episode_start_;
    Timestamp episode_end_;

    /// Parsed procedures from any procedure field
    std::set<std::string> procedures_;

    /// Parsed diagnoses from any diagnosis field
    std::set<std::string> diagnoses_;

    Integer age_at_episode_; 
};


// A spell is a hospital visit. The spell may contain multiple
// episodes if the hospital stay involved multiple consultants.
class Spell {
public:
    Spell(RowBuffer auto & row, CodeParser & code_parser) {
	// Assume the next row is the start of a new spell
	// block. Push back to the episodes vector one row
	// per episode.

	// The first row contains the spell id
	spell_id_ = column<Varchar>("spell_id", row).read();
	spell_start_ = column<Timestamp>("spell_start", row);
	spell_end_ = column<Timestamp>("spell_end", row);
	
	while (column<Varchar>("spell_id", row).read() == spell_id_) {

	    // If you get here, then the current row
	    // contains an episode that is part of this
	    // spell.

	    /// Note this will consume a row and fetch the
	    /// next row
	    Episode episode{row, code_parser};

	    // Only include this episode in the list if it
	    // contains some diagnoses or procedures
	    if (not episode.empty()) {
		episodes_.push_back(episode);
	    }
	}
    }

    /// If the spell contains no episodes, then it is
    /// considered empty
    bool empty() const {
	return episodes_.empty();
    }

    const auto & episodes() const {
	return episodes_;
    }

    void print() const {
	std::cout << " Spell " << spell_id_ << std::endl << "  ";
	spell_start_.print();
	std::cout << " - ";
	spell_end_.print();
	std::cout << std::endl;
	for (const auto & episode : episodes_) {
	    episode.print();
	}
    }
 
private:
    std::string spell_id_;
    Timestamp spell_start_;
    Timestamp spell_end_;
    std::vector<Episode> episodes_;
};

/// If all three of the mortality fields are NULL, then the
/// patient is considered still alive.
bool patient_alive(const RowBuffer auto & row) {
    auto date_of_death{column<Timestamp>("date_of_death", row)};
    auto cause_of_death{column<Varchar>("cause_of_death", row)};
    auto age_at_death{column<Integer>("age_at_death", row)};

    return date_of_death.null()
	and cause_of_death.null()
	and age_at_death.null();
}
    
class Patient {
public:
    /// The row object passed in has _already had the
    /// first row fetched_. At the other end, when it
    /// discovers a new patients, the row is left in
    /// the buffer for the next Patient object
    Patient(RowBuffer auto & row, CodeParser & code_parser)
	: alive_{patient_alive(row)} {

	// The first row contains the nhs number
	nhs_number_ = column<Integer>("nhs_number", row).read();
	
	while(column<Integer>("nhs_number", row).read() == nhs_number_) {

	    // If you get here, then the current row
	    // contains valid data for this patient

	    // Collect a block of rows into a spell.
	    // Note that this will leave row pointing
	    // to the start of the next spell block
	    Spell spell{row, code_parser};

	    // If the spell is not empty, include it in
	    // the list of spells
	    if (not spell.empty()) {
		spells_.push_back(spell);
	    }

	    if (not alive_) {
		cause_of_death_ = code_parser.cause_of_death(row);
	    }
	}
    }

    auto nhs_number() const {
	return nhs_number_;
    }
    
    const auto & spells() const {
	return spells_;
    }
    
    /// Returns true if none of the spells contain any
    /// diagnoses or procedures in the code groups file 
    bool empty() const {
	return spells_.empty();
    }

    void print() const {
	std::cout << "Patient: " << nhs_number_ << std::endl;
	for (const auto & spell : spells_) {
	    spell.print();
	}
	if (not alive_) {
	    std::cout << " Cause of death: ";
	    for (const auto & cause : cause_of_death_) {
		std::cout << cause << ",";
	    }
	    std::cout << std::endl;
	}
    }    
    
private:
    long long unsigned nhs_number_;
    std::vector<Spell> spells_;
   
    bool alive_;
    std::set<std::string> cause_of_death_;
    
};

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
	       const std::set<std::string> & index_procedures) {

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

    Timestamp date() const {
	return date_;
    }

    /// Print the index event
    void print() const {
	std::cout << "Index: ";
	date_.print();
	std::cout << " Procedure? " << procedure_triggered_;
	std::cout << "; Age: " << age_at_index_.value_or(-1);
    }
private:
    /// True if a procedure generated the index event, false otherwise
    bool procedure_triggered_;
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

const std::string episodes_query{
    R"raw_sql(

select top 5000
	episodes.*,
	mort.REG_DATE_OF_DEATH as date_of_death,
	mort.S_UNDERLYING_COD_ICD10 as cause_of_death,
	mort.Dec_Age_At_Death as age_at_death
from (
	select
		AIMTC_Pseudo_NHS as nhs_number,
                AIMTC_Age as age_at_episode,
		PBRspellID as spell_id,
		StartDate_ConsultantEpisode as episode_start,
		EndDate_ConsultantEpisode as episode_end,
		AIMTC_ProviderSpell_Start_Date as spell_start,
		AIMTC_ProviderSpell_End_Date as spell_end,
		diagnosisprimary_icd,
		diagnosis1stsecondary_icd,
		diagnosis2ndsecondary_icd,
		primaryprocedure_opcs,
		procedure2nd_opcs,
		procedure3rd_opcs,
		procedure4th_opcs,
		procedure5th_opcs,
		procedure6th_opcs
	from abi.dbo.vw_apc_sem_001
	where datalength(AIMTC_Pseudo_NHS) > 0
		and datalength(pbrspellid) > 0  
) as episodes
left join abi.civil_registration.mortality as mort
on episodes.nhs_number = mort.derived_pseudo_nhs
order by nhs_number, spell_id;

    )raw_sql"
};


std::vector<Record> get_acs_records(const YAML::Node & config) {

    std::vector<Record> records;
    
    // Contains the parsers for OPCS and ICD codes
    CodeParser code_parser{config["parser_config"]};

    // Load the index diagnosis and procedures lists
    auto include{config["index_event"]["include"]};
    auto index_diagnoses{expect_string_set(include, "diagnoses")};
    auto index_procedures{expect_string_set(include, "procedures")};
	
    // Fetch the database name and connect
    auto dsn{config["data_sources"]["dsn"].as<std::string>()};
    std::cout << "Connection to DSN " << dsn << std::endl;
    SQLConnection con{dsn};
    std::cout << "Executing statement" << std::endl;
    auto row{con.execute_direct(episodes_query)};
        
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
						   index_procedures};
			    // For this index event, construct the
			    // record by looking at at all episodes
			    // before/after the index event for this
			    // patient
			    Record record{patient, index_event};
			    record.print();

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

    return records;
}

#endif
