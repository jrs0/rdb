#ifndef PATIENT_HPP
#define PATIENT_HPP

#include "row_buffer.hpp"
#include "spell.hpp"
#include <ostream>

class Mortality {
public:

    struct PatientAlive {};
    
    Mortality(const RowBuffer auto & row, std::shared_ptr<ClinicalCodeParser> parser) {
	date_of_death_ = column<Timestamp>("date_of_death", row);
	age_at_death_ = column<Integer>("age_at_death", row);
	auto cause_of_death_raw_{column<Varchar>("cause_of_death", row)};
	
	if (date_of_death_.null()
	    and age_at_death_.null()
	    and cause_of_death_raw_.null()) {
	    alive_ = true;
	}

	if (not cause_of_death_raw_.null()) {
	    cause_of_death_ = parser->parse(CodeType::Diagnosis, cause_of_death_raw_.read());
	}
    }
    
    auto alive() const {
	return alive_;
    }

    auto cause_of_death() const {
	if (alive()) {
	    throw PatientAlive{};
	}
	return cause_of_death_;
    }

    auto age_at_death() const {
	if (alive()) {
	    throw PatientAlive{};
	}
	return age_at_death_;
    }

    auto date_of_death() const {
	if (alive()) {
	    throw PatientAlive{};
	}
	return date_of_death_;
    }

    void print(std::shared_ptr<StringLookup> lookup, std::size_t pad = 0) const {
	
	std::cout << std::string(pad, ' ')
		  << "Mortality: ";
	if (alive()) {
	    std::cout << "alive" << std::endl;
	} else {
	    std::cout << std::endl
		      << std::string(pad, ' ') << "- date of death = " << date_of_death_ << std::endl
		      << std::string(pad, ' ') << "- age at death = " << age_at_death_  << std::endl
		      << std::string(pad, ' ') << "- cause of death = ";
	    if (cause_of_death_) {
		::print(cause_of_death_.value(), lookup);
		std::cout << std::endl;
	    } else {
		std::cout << "Unknown" << std::endl;
	    }
	}	    
    }
    
private:
    std::optional<ClinicalCode> cause_of_death_;
    Integer age_at_death_;
    Timestamp date_of_death_;
    bool alive_{false};
};

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

    void print(std::shared_ptr<StringLookup> lookup, std::size_t pad = 0) const {
	std::cout << Colour::PINK <<"Patient: " << nhs_number_
		  << Colour::RESET << std::endl;
	mortality_.print(lookup, pad);
	for (const auto & spell : spells_) {
	    spell.print(lookup, pad + 4);
	}
    }    
    
private:
    Mortality mortality_;
    long long unsigned nhs_number_;
    std::vector<Spell> spells_;
};

#endif
