#ifndef PATIENT_HPP
#define PATIENT_HPP

#include "row_buffer.hpp"
#include "spell.hpp"
#include <ostream>
#include "mortality.hpp"

class Patient {
public:
    /// The row object passed in has _already had the
    /// first row fetched_. At the other end, when it
    /// discovers a new patients, the row is left in
    /// the buffer for the next Patient object
    Patient(RowBuffer auto & row, std::shared_ptr<ClinicalCodeParser> parser)
	// Take the mortality data from the first row of the first spell, because
	// the mortality table was left-joined (so all rows will be the same)
	: mortality_{row, parser} {

	try {
	    nhs_number_ = column<Integer>("nhs_number", row).read();
	} catch (const RowBufferException::ColumnNotFound &) {
	    throw std::runtime_error("Missing required nhs_number column in Patient constructor");
	} catch (const RowBufferException::WrongColumnType &) {
	    throw std::runtime_error("Wrong column type for nhs_number in Patient constructor");
	}
	    
	while(column<Integer>("nhs_number", row).read() == nhs_number_) {
	    spells_.emplace_back(row, parser);
	}
    }

    auto nhs_number() const {
	return nhs_number_;
    }
    
    const auto & spells() const {
	return spells_;
    }

    const auto & mortality() const {
	return mortality_;
    }

    void print(std::ostream & os, std::shared_ptr<StringLookup> lookup, std::size_t pad = 0) const {
	os << Colour::PINK <<"Patient: " << nhs_number_
		  << Colour::RESET << std::endl;
	mortality_.print(os, lookup, pad);
	for (const auto & spell : spells_) {
	    spell.print(os, lookup, pad + 4);
	}
    }    
    
private:
    Mortality mortality_;
    long long unsigned nhs_number_;
    std::vector<Spell> spells_;
};

#endif
