#ifndef SPELL_HPP
#define SPELL_HPP

#include "episode.hpp"
#include "row_buffer.hpp"

class Spell {
public:
    Spell(RowBuffer auto & row, std::shared_ptr<ClinicalCodeParser> parser) {
	// Assume the next row is the start of a new spell
	// block. Push back to the episodes vector one row
	// per episode.

	// The first row contains the spell id
	try {
	    spell_id_ = column<Varchar>("spell_id", row).read();
	    spell_start_ = column<Timestamp>("spell_start", row);
            spell_end_ = column<Timestamp>("spell_end", row);
	} catch (const RowBufferException::ColumnNotFound &) {
	    throw std::runtime_error("Missing required column in Spell constructor");
	} catch (const RowBufferException::WrongColumnType &) {
	    throw std::runtime_error("Column type errors in Spell constructor");
	}

	try {
	    while (column<Varchar>("spell_id", row).read() == spell_id_) {
		episodes_.push_back(Episode{row, parser});
		row.fetch_next_row();
	    }
	} catch (const RowBufferException::NoMoreRows & e) {
	    sort_episodes();
	    throw;
	}
	
	sort_episodes();
    }

    /// Sort the episodes by start date
    void sort_episodes() {
        std::ranges::sort(episodes_, {}, &Episode::episode_start);	
    }
    
    /// If the spell contains no episodes, then it is
    /// considered empty
    bool empty() const {
	return episodes_.empty();
    }

    const auto & episodes() const {
	return episodes_;
    }

    /// Return the spell start date, or fall back
    /// to the start date of the first episode. If
    /// that is empty, return null
    auto start_date() const {
	if (not spell_start_.null()) {
	    return spell_start_;
	} else if (not episodes_.empty()){
	    return episodes_[0].episode_start();
	} else {
	    return Timestamp{};
	}
    }
    
    void print(std::shared_ptr<StringLookup> lookup, std::size_t pad = 0) const {
	std::cout << std::string(pad, ' ');
	std::cout << "Spell " << spell_id_ << std::endl;
	std::cout << std::string(pad, ' ');
	spell_start_.print();
	std::cout << " - ";
	spell_end_.print();
	std::cout << std::endl << std::endl;
	for (const auto & episode : episodes_) {
	    episode.print(lookup, pad + 4);
	    std::cout << std::endl;
	}
    }
    
private:
    std::string spell_id_;
    Timestamp spell_start_;
    Timestamp spell_end_;
    std::vector<Episode> episodes_;
};

#endif
