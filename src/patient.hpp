#ifndef PATIENT_HPP
#define PATIENT_HPP

#include "row_buffer.hpp"
#include "spell.hpp"

/// If all three of the mortality fields are NULL, then the
/// patient is considered still alive.
bool patient_alive(const RowBuffer auto & row) {

    /*

    */

    // For now not using mortality
    return true;
}

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

    auto date_at_death() const {
	if (alive()) {
	    throw PatientAlive{};
	}
	return age_at_death_;
    }

    void print(std::shared_ptr<StringLookup> lookup, std::size_t pad = 0) const {
	
	//std::cout << ""
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
