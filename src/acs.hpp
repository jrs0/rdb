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

#include <vector>
#include <ranges>
#include "patient.hpp"
#include "episode.hpp"
#include "spell.hpp"
#include "clinical_code.hpp"

/** 
 * \brief Stores the data for a row in the ACS dataset
 *
 * Each ACS record is triggered by an index event which is
 * either and ACS diagnosis or a PCI procedure. The class also
 * records relevant events that occur before and after (currently
 * hardcoded to 12 months, should be defined in the config file).
 */
class AcsRecord {
public:

    AcsRecord(const Patient & patient, const Spell & index_spell) {
	nhs_number_ = patient.nhs_number();
	auto & first_episode{index_spell.episodes()[0]};
	age_at_index_ = first_episode.age_at_episode();
	date_of_index_ = first_episode.episode_start();
	
    }
    
    /// Increment a group counter in the before map
    void push_before(const ClinicalCodeGroup & group) {
	before_counts_[group]++;
    }

    void push_after(const ClinicalCodeGroup & group) {
	after_counts_[group]++;
    }

    void set_death_after(const Mortality & mortality, const ClinicalCodeMetagroup & acs_group) {

	if (not mortality.alive()) {

	    auto date_of_death{mortality.date_of_death()};
	    if (not date_of_death.null() and not date_of_index_.null()) {

		if (date_of_death < date_of_index_) {
		    throw std::runtime_error("Unexpected date of death before index date at patient"
					     + std::to_string(nhs_number_));
		}

		// Check if death occurs in window after (hardcoded for now)
		index_to_death_ = date_of_death - date_of_index_;
		if (index_to_death_.value() < years(1)) {

		    death_after_ = true;

		    auto cause_of_death{mortality.cause_of_death()};
		    if (cause_of_death.has_value()) {
			acs_death_ = acs_group.contains(cause_of_death.value());

			// Also increment the relevant counter if the death is in
			// the acs group, so it is recorded as a post index event too.
                        if (acs_death_) {
			    for (const auto & group : cause_of_death.value().groups()) {
				push_after(group);
			    }
			}
		    }
		}
	    }
	}
    }
    
    void print(std::shared_ptr<StringLookup> lookup) const {
	std::cout << "ACS Record for NHS number " << nhs_number_ << std::endl;
	std::cout << "Age at index: " << age_at_index_ << std::endl;
	std::cout << "- Counts before:" << std::endl;
	for (const auto & [group, count] : before_counts_) {
	    std::cout << "  - ";
	    group.print(lookup);
	    std::cout << ": " << count
		      << std::endl;
	}
	std::cout << "- Counts after:" << std::endl;
	for (const auto & [group, count] : after_counts_) {
	    std::cout << "  - ";
	    group.print(lookup);
	    std::cout << ": " << count
		      << std::endl;
	}
	if (not death_after_) {
	    std::cout << "No death after" << std::endl;
	} else {
	    std::cout << "Death occurred ";
	    if (index_to_death_.has_value()) {
		std::cout << index_to_death_.value();
	    } else {
		std::cout << "unknown";
	    }
	    std::cout << " seconds after (";
            if (acs_death_) {
		std::cout << "ACS-cause)";
	    } else {
		std::cout << "all-cause)";
	    }
	    std::cout << std::endl;
	}
    }

private:
    long long unsigned nhs_number_;
    Integer age_at_index_;
    Timestamp date_of_index_;
    std::map<ClinicalCodeGroup, std::size_t> before_counts_;
    std::map<ClinicalCodeGroup, std::size_t> after_counts_;
    bool death_after_{false};

    /// False means all cause or unknown, true means the cause of
    /// death diagnosis was the acs metagroup
    bool acs_death_{false};
    std::optional<TimestampOffset> index_to_death_;    
};

/// Is index event if there is a primary ACS or PCI
/// in the _first_ episode of the spell
auto get_acs_index_spells(const std::vector<Spell> & spells,
			  const ClinicalCodeMetagroup & acs_group,
			  const ClinicalCodeMetagroup & pci_group) {
    auto is_acs_index_spell{[&](const Spell &spell) {
	auto & episodes{spell.episodes()};
	if (episodes.empty()) {
	    return false;
	}
	auto & first_episode{episodes[0]};
	auto primary_acs{acs_group.contains(first_episode.primary_diagnosis())};
	auto primary_pci{pci_group.contains(first_episode.primary_procedure())};
	return primary_acs or primary_pci;
    }};
    
    return spells | std::views::filter(is_acs_index_spell);
}

/// Get secondary lists from the first episode of the index spell
auto get_index_secondaries(const Spell & index_spell, CodeType type) {
    return index_spell.episodes() |
	std::views::take(1) |
	std::views::transform([=](const auto & episode) {
	    return episode.secondaries(type);
	}) |
	std::views::join |
	std::views::filter(&ClinicalCode::valid) |
	std::views::transform(&ClinicalCode::groups) |
	std::views::join;
}


/// Get all the spells whose start date is strictly between
/// the time of the base spell and an offset in seconds (positive
/// for after, negative for before). The base spell is not included.
auto get_spells_in_window(const std::vector<Spell> & all_spells,
			  const Spell & base_spell,
			  int offset_seconds) {
    auto in_window{[&base_spell, offset_seconds](const Spell & other_spell) {
	auto base_start{base_spell.start_date()};
	auto other_spell_start{other_spell.start_date()};

	if (offset_seconds > 0) {
	    return (other_spell_start > base_start)
		and (other_spell_start < base_start + offset_seconds);	    
	} else {
	    return (other_spell_start < base_start)
		and (other_spell_start > base_start + offset_seconds);
	}
    }};
		
    // Get the spells that occur before the index event
    return all_spells |
	std::views::filter(in_window);

}

/// Fetch all the code groups present in the primary
/// and secondary diagnoses and procedures of all the
/// episodes in a vector of spells
auto get_all_groups(std::ranges::range auto && spells) {
    return spells |
	std::views::transform(&Spell::episodes) |
	std::views::join |
	std::views::transform(&Episode::all_procedures_and_diagnosis) |
	std::views::join |
	std::views::filter(&ClinicalCode::valid) |
	std::views::transform(&ClinicalCode::groups) |
	std::views::join;
}

auto get_record_from_index_spell(const Patient & patient,
				 const Spell & index_spell,
				 const ClinicalCodeMetagroup acs_group,
				 std::shared_ptr<StringLookup> lookup,
				 bool print) {
    AcsRecord record{patient, index_spell};
    
    // Do not add secondary procedures into the counts, because they
    // often represent the current index procedure (not prior procedures)
    for (const auto & group : get_index_secondaries(index_spell, CodeType::Diagnosis)) {
	record.push_before(group);
    }

    auto spells_before{get_spells_in_window(patient.spells(), index_spell, -365*24*60*60)};

    for (const auto & group : get_all_groups(spells_before)) {
	record.push_before(group);
    }

    auto spells_after{get_spells_in_window(patient.spells(), index_spell, 365*24*60*60)};
    for (const auto & group : get_all_groups(spells_after)) {
	record.push_after(group);
    }

    record.set_death_after(patient.mortality(), acs_group);
    
    if (print) {
	std::cout << "INDEX SPELL:" << std::endl;
	index_spell.print(lookup, 4);

	std::cout << "SPELLS BEFORE INDEX:" << std::endl;
	for (const auto & spell_before : spells_before) {
	    spell_before.print(lookup, 4);
	}
	
	std::cout << "SPELLS AFTER INDEX:" << std::endl;
	for (const auto & spell_after : spells_after) {
	    spell_after.print(lookup, 4);
	}
    
	std::cout << "INDEX RECORD:" << std::endl;
	record.print(lookup);
	std::cout << std::endl;
    }


    
    return record;
}


#endif
