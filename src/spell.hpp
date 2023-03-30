#ifndef SPELL_HPP
#define SPELL_HPP

#include "episode.hpp"

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

#endif
