// This file interprets hospital episode data into records
// containing an index event, and counts of certain events
// that occur before and after the index event.
//
// For the purposes of this file, an episode is only of interest
// if its diagnosis or procedure falls within a group of
// interest. If it does not, then that episode can be ignored.

#ifndef ACS_HPP
#define ACS_HPP

#include <yaml-cpp/yaml.h>
#include "category.hpp"

/// This class has two jobs -- keep the diagnoses and
/// procedure parsers close together; and map collections
/// of groups as defined in the codes file into "meta" groups
/// that will become the column names in the data frame.
///
/// This class owns its parser, so it will be thread safe
/// at the cost of duplicated copies of the codes tree
class CodeParser {
public:
    CodeParser(const YAML::Node & parser_config) {
	// Open both the codes files and make the
	// categories -- also store maps that group
	// together categories into higher level groups,
	// as defined by the parser_config
    }

    /// Parse a procedure, returning the code group defined
    /// by the parser_config (this may group multiple
    /// code groups defined in the codes file together) 
    std::string parse_procedure(const std::string & procedure) {

    }

    /// Parse a diagnosis, return code group
    std::string parse_diagnosis(const std::string & diagnosis) {

    }
    
private:
    TopLevelCategory procedures_category_;
    std::map<std::string, std::string> procedures_group_map_;
    
    TopLevelCategory diagnoses_category_;
    std::map<std::string, std::string> diagnoses_group_map_;
};

/// The purpose of the Episode class is to parse all the
/// primary and secondary diagnosis and procedure fields
/// and expose them as flat lists of groups (defined by
/// the ICD and OPCS code group files)
class Episode {
public:
    Episode() {
	// Expect a single row as argument, which will be the
	// episode. THe episode may contain either a procedure
	// or a diagnosis (including secondaries), or both. The
	// episode relates to what happened under one attending
	// consultant.

	// Need to parse all the primary and secondary fields
	// here using the codes files, so there needs to be
	// a reference to a top_level_category for ICD and OPCS,
	// and also the config file for further grouping. This
	// can probably be abstracted into a wrapper which
	// handles the parsing and mapping 
    }

    std::vector<std::string> procedures() const {
	return procedure_groups_;
    }

    std::vector<std::string> diagnoses() const {
	return diagnosis_groups_;
    }
    
private:
    std::string episode_start_;
    std::string episode_end_;
    std::string episode_id_;

    std::vector<std::string> diagnosis_groups_;
    std::vector<std::string> procedure_groups_;
};


// A spell is a hospital visit. The spell may contain multiple
// episodes if the hospital stay involved multiple consultants.
class Spell {
public:
    Spell() {
	// Assume the next row is the start of a new spell
	// block. Push back to the episodes vector one row
	// per episode.
    }

private:
    std::string spell_start_;
    std::string spell_end_;
    std::string spell_id_;
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


class Acs {
public:
    Acs(const YAML::Node & category) {

	// sql statement that fetches all episodes for all patients
	// ordered by nhs number, then spell id. Include count column
	// for length of patient, length of spell.

	// While true, keep fetching into Record, push back results
	// Record constructor takes reference to results, and uses
	// up one patient block per record
	
    }
    
private:
    std::vector<Record> records_;

};



#endif
