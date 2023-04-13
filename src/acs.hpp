#ifndef ACS_HPP
#define ACS_HPP

#include <vector>
#include <ranges>
#include "patient.hpp"
#include "episode.hpp"
#include "spell.hpp"
#include "clinical_code.hpp"
#include "event_counter.hpp"

const auto & get_first_episode(const Spell & spell) {
    if (spell.episodes().empty()) {
	throw std::runtime_error("Spell has no episodes in call to first_episode()");
    }
    return spell.episodes()[0];
}

auto primary_acs(const Episode & episode, const ClinicalCodeMetagroup & acs_group) {
    return acs_group.contains(episode.primary_diagnosis());    
}

auto primary_pci(const Episode & episode, const ClinicalCodeMetagroup & pci_group) {
    return pci_group.contains(episode.primary_procedure());
}


/// Is index event if there is a primary ACS or PCI
/// in the _first_ episode of the spell
auto get_acs_and_pci_spells(const std::vector<Spell> & spells,
			  const ClinicalCodeMetagroup & acs_group,
			  const ClinicalCodeMetagroup & pci_group) {
    auto is_acs_index_spell{[&](const Spell &spell) {
	auto & episodes{spell.episodes()};
	if (episodes.empty()) {
	    return false;
	}
	auto & first_episode{episodes[0]};
	return primary_acs(first_episode, acs_group)
	    or primary_pci(first_episode, pci_group);
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
    return spells | std::views::transform(&Spell::episodes) | std::views::join |
	std::views::transform(&Episode::all_procedures_and_diagnosis) |
	std::views::join |
	std::views::filter(&ClinicalCode::valid) |
	std::views::transform(&ClinicalCode::groups) |
	// BUG: removes duplicates incorrectly before count (std::set)
	std::views::join;
}

/// Returns true if the index_spell was a stemi
auto get_stemi_presentation(const Spell & index_spell,
			    const ClinicalCodeMetagroup & stemi_group) {
    
    auto index_codes{
	index_spell.episodes() |
	std::views::transform(&Episode::all_procedures_and_diagnosis) |
	std::views::join |
	std::views::filter(&ClinicalCode::valid)
    };
    return std::ranges::any_of(index_codes,
			       [&](const auto & code) {
			       return stemi_group.contains(code);
			       });
}

#endif
