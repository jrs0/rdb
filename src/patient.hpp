#ifndef PATIENT_HPP
#define PATIENT_HPP

#include "spell.hpp"

/// If all three of the mortality fields are NULL, then the
/// patient is considered still alive.
bool patient_alive(const RowBuffer auto & row) {

    /*
      auto date_of_death{column<Timestamp>("date_of_death", row)};
      auto cause_of_death{column<Varchar>("cause_of_death", row)};
      auto age_at_death{column<Integer>("age_at_death", row)};

      return date_of_death.null()
      and cause_of_death.null()
      and age_at_death.null();

    */

    // For now not using mortality
    return true;
}
    
class Patient {
public:
    /// The row object passed in has _already had the
    /// first row fetched_. At the other end, when it
    /// discovers a new patients, the row is left in
    /// the buffer for the next Patient object
    Patient(RowBuffer auto & row, CodeParser & code_parser)
	: alive_{patient_alive(row)} {

	// The first row contains the nhs number
	nhs_number_ = column<Integer>("nhs_number", row).read();
	
	while(column<Integer>("nhs_number", row).read() == nhs_number_) {

	    // If you get here, then the current row
	    // contains valid data for this patient

	    // Collect a block of rows into a spell.
	    // Note that this will leave row pointing
	    // to the start of the next spell block
	    Spell spell{row, code_parser};

	    // If the spell is not empty, include it in
	    // the list of spells
	    if (not spell.empty()) {
		spells_.push_back(spell);
	    }

	    if (not alive_) {
		cause_of_death_ = code_parser.cause_of_death(row);
	    }
	}
    }

    auto nhs_number() const {
	return nhs_number_;
    }
    
    const auto & spells() const {
	return spells_;
    }
    
    /// Returns true if none of the spells contain any
    /// diagnoses or procedures in the code groups file 
    bool empty() const {
	return spells_.empty();
    }

    bool alive() const {
	return alive_;
    }
    
    void print() const {
	std::cout << "Patient: " << nhs_number_ << std::endl;
	for (const auto & spell : spells_) {
	    spell.print();
	}
	if (not alive_) {
	    std::cout << " Cause of death: ";
	    for (const auto & cause : cause_of_death_) {
		std::cout << cause << ",";
	    }
	    std::cout << std::endl;
	}
    }    
    
private:
    long long unsigned nhs_number_;
    std::vector<Spell> spells_;
   
    bool alive_;
    std::set<std::string> cause_of_death_;
    
};

#endif
