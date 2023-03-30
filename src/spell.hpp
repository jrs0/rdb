#ifndef SPELL_HPP
#define SPELL_HPP

#include "episode.hpp"

class Spell {
public:
    Spell(RowBuffer auto & row, ClinicalCodeParser & parser) {
	// Assume the next row is the start of a new spell
	// block. Push back to the episodes vector one row
	// per episode.

	// The first row contains the spell id
	try {
	    spell_id_ = "ABCDE";//column<Varchar>("spell_id", row).read();
	    spell_start_ = Timestamp{123};//column<Timestamp>("spell_start", row);
	    spell_end_ = Timestamp{126};//column<Timestamp>("spell_end", row);
	} catch (const std::out_of_range &) {
	    throw std::runtime_error("Missing required column in Spell constructor");
	} catch (const std::bad_variant_access &) {
	    throw std::runtime_error("Column type errors in Spell constructor");
	}

	    
	while (column<Varchar>("spell_id", row).read() == spell_id_) {
	    episodes_.push_back(Episode{row, parser});
	    row.fetch_next_row();	    
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

    void print(const ClinicalCodeParser & parser, std::size_t pad = 0) const {
	std::cout << " Spell " << spell_id_ << std::endl << "  ";
	spell_start_.print();
	std::cout << " - ";
	spell_end_.print();
	std::cout << std::endl;
	for (const auto & episode : episodes_) {
	    episode.print(parser, pad + 2);
	}
    }
    
private:
    std::string spell_id_;
    Timestamp spell_start_;
    Timestamp spell_end_;
    std::vector<Episode> episodes_;
};

#endif
