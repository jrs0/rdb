#ifndef MORTALITY_HPP
#define MORTALITY_HPP

#include "row_buffer.hpp"
#include "clinical_code.hpp"

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

#endif