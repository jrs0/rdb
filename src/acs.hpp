#ifndef ACS_HPP
#define ACS_HPP

#include <yaml-cpp/yaml.h>

class Episode {
public:
    Episode() {}
private:
    std::string episode_start_;
    std::string episode_end_;
    std::string episode_id_;
};

class Spell {
public:
    Spell() {}
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

	// 
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
