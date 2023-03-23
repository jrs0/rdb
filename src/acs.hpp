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

#include <yaml-cpp/yaml.h>
#include "category.hpp"
#include "sql_connection.hpp"

/// Load a top_level_category YAML Node from file
YAML::Node load_codes_helper(const YAML::Node & config) {
    try {
	return YAML::LoadFile(config["file"].as<std::string>());
    } catch(const YAML::BadFile& e) {
	throw std::runtime_error("Bad YAML file");
    } catch(const YAML::ParserException& e) {
	throw std::runtime_error("YAML parsing error");
    }
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
std::vector<std::string> all_codes(const std::vector<std::string> & columns,
				   const RowBuffer auto & row,
				   TopLevelCategory & parser) {
    std::vector<std::string> result;
    for (const auto & column : columns) {
	try {
	    auto value{row.at(column)};
	    auto parsed{parser.get_code_prop(value, false)};
	    result.push_back(parsed);
	} catch (const std::runtime_error & /* invalid or not found */) {
	    // Continue
	} catch (const std::out_of_range & e) {
	    throw std::runtime_error("Could not read from column "
				     + column);
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
	: procedures_{load_codes_helper(parser_config["procedures"])},
	  diagnoses_{load_codes_helper(parser_config["diagnoses"])},
	  procedure_columns_{source_columns(parser_config["procedures"])},
	  diagnosis_columns_{source_columns(parser_config["diagnoses"])}
    {
	// Open both the codes files and make the
	// categories -- also store maps that group
	// together categories into higher level groups,
	// as defined by the parser_config
    }
    
    /// Parse all the procedure codes into a flat list, omitting
    /// any codes that are invalid. In addition, map the parsed
    std::vector<std::string> all_procedures(const RowBuffer auto & row) {
	return all_codes(procedure_columns_, row, procedures_);
    }

    /// Parse all the diagnosis codes
    std::vector<std::string> all_diagnoses(const RowBuffer auto & row) {
	return all_codes(diagnosis_columns_, row, diagnoses_);
    }
    
private:
    
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
/// the ICD and OPCS code group files)
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

	procedures_ = code_parser.all_procedures(row);
	diagnoses_ = code_parser.all_diagnoses(row);
	
	row.fetch_next_row();
    }

    std::vector<std::string> procedures() const {
	return procedures_;
    }

    std::vector<std::string> diagnoses() const {
	return diagnoses_;
    }

    void print() const {
	std::cout << "  Episode: D(";
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
    std::string episode_start_;
    std::string episode_end_;
    std::string episode_id_;

    /// Parsed procedures from any procedure field
    std::vector<std::string> procedures_;

    /// Parsed diagnoses from any diagnosis field
    std::vector<std::string> diagnoses_;
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
	try {
	    spell_id_ = row.at("spell_id");
	} catch (const std::out_of_range & e) {
	    throw std::runtime_error("Column not found");
	}

       	std::cout << " Spell " << spell_id_ << std::endl;
	while (row.at("spell_id") == spell_id_) {

	    // If you get here, then the current row
	    // contains an episode that is part of this
	    // spell.

	    /// Note this will consume a row and fetch the
	    /// next row
	    episodes_.emplace_back(row, code_parser);
	    episodes_.back().print();
	}
    }

private:
    std::string spell_id_;
    std::string spell_start_;
    std::string spell_end_;
    std::vector<Episode> episodes_;
};

class IndexEvent {
public:
    IndexEvent() {}
private:
    bool event_type_; ///< true for acs, false for pci
    std::string date_;
    std::size_t age_at_index_; ///< Age at the index event
};

/// An index event along with events before and
/// after
class Record {
public:
    Record() {
	// Assume rows ordered by patient. Loop over rows
	// until next patient is found (needs a count of
	// some kind, otherwise will use up first row of
	// next patient.	

	// Make a vector of spells. Pass results object by
	// reference to spells constructor and allow it to
	// consume all the episodes in the spell.
	std::vector<Spell> spells;

	// Process the list of spells. The target information
	// is the identification of index events, which will
	// be one to one with Records, and counting before
	// and after events. 
    }
private:
    IndexEvent index_event_;
    std::map<std::string, std::size_t> num_events_before_;
    std::map<std::string, std::size_t> num_events_after_;
    
};

class Patient {
public:
    /// The row object passed in has _already had the
    /// first row fetched_. At the other end, when it
    /// discovers a new patients, the row is left in
    /// the buffer for the next Patient object
    Patient(RowBuffer auto & row, CodeParser & code_parser) {

	// The first row contains the nhs number
	try {
	    nhs_number_ = row.at("nhs_number");
	} catch (const std::out_of_range & e) {
	    throw std::runtime_error("Column not found");
	}

	std::cout << "Patient " << nhs_number_ << std::endl;
	while(row.at("nhs_number") == nhs_number_) {

	    // If you get here, then the current row
	    // contains valid data for this patient

	    // Store the patient info. Note that this
	    // will leave row pointing to the start of
	    // the next spell block
	    spells_.emplace_back(row, code_parser);
	}
    }

private:
    std::string nhs_number_;
    std::vector<Spell> spells_;
};

const std::string episodes_query{
    R"raw_sql(

select top 500
	episodes.*,
	mort.REG_DATE_OF_DEATH as date_of_death,
	mort.S_UNDERLYING_COD_ICD10 as cause_of_death,
	mort.Dec_Age_At_Death as age_at_death
from (
	select
		AIMTC_Pseudo_NHS as nhs_number,
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

class Acs {
public:
    Acs(const YAML::Node & config)
	: code_parser_{config["parser_config"]}
    {
	// Fetch the database name and connect
	auto dsn{config["data_sources"]["dsn"].as<std::string>()};
	std::cout << "Connection to DSN " << dsn << std::endl;
	SQLConnection con{dsn};
	std::cout << "Executing statement" << std::endl;
	auto row{con.execute_direct(episodes_query)};
	std::cout << "Starting to fetch rows" << std::endl;

	// sql statement that fetches all episodes for all patients
	// ordered by nhs number, then spell id.

	// Print the returned rows
	for (const auto & column_name : row.column_names()) {
	    std::cout << column_name << std::endl;
	}
	
	// Make the first fetch
	row.fetch_next_row();
	
	while (true) {
	    try {
		patients_.emplace_back(row, code_parser_);
	    } catch (const std::logic_error & e) {
		// There are no more rows
		std::cout << "No more rows -- finished" << std::endl;
		break;
	    }
	}
	
	// While true, keep fetching into Record, push back results
	// Record constructor takes reference to results, and uses
	// up one patient block per record
	
    }
    
private:
    std::vector<Patient> patients_;
    CodeParser code_parser_;
};



#endif
