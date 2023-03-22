#ifndef ACS_HPP
#define ACS_HPP

#include <yaml-cpp/yaml.h>

class Episode {
public:
    Episode() {
	// Expect a single row as argument, which will be the
	// episode. THe episode may contain either a procedure
	// or a diagnosis (including secondaries), or both. The
	// episode relates to what happened under one attending
	// consultant.
    }
private:
    std::string episode_start_;
    std::string episode_end_;
    std::string episode_id_;
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
