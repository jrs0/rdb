#ifndef EPISODE_HPP
#define EPISODE_HPP

#include "row_buffer.hpp"
#include "category.hpp"

#include "set_utils.hpp"
#include "clinical_code.hpp"

#include "sql_types.hpp"

class Episode {
public:

    /// Create an episode with all empty (null) fields
    Episode() = default;

    /// Read the data in the row into the episode structue. Assume
    /// that the following fields are present in the row:
    ///
    /// - episode_start (Timestamp) - contains the episode start time
    /// - episode_end (Timestamp) - contains the episode end time
    /// - primary_diagnosis (Varchar) - the primary diagnosis
    /// - primary_procedure (Varchar) - the primary procedure
    /// - secondary_diagnosis_<n> (Varchar) - the nth secondary diagnosis
    /// - secondary_procedure_<n> (Varchar) - the nth secondary procedure
    /// 
    /// <n> is a non-negative integer. The first column that is not found
    /// signals the end of the block of secondary columns. The function will
    /// short circuit on a NULL or empty (whitespace) secondary column.
    Episode(RowBuffer auto & row, ClinicalCodeParser & parser) {

        episode_start_ = column<Timestamp>("episode_start", row);
        episode_end_ = column<Timestamp>("episode_end", row);

	// Get primary procedure
	try {
	    auto raw{column<Varchar>("primary_procedure", row).read()};
	    primary_procedure_ = parser.parse_procedure(raw);
	} catch (const std::out_of_range &) {
	    throw std::runtime_error("Missing required 'primary_procedure' column in row");
	} catch (const std::bad_variant_access &) {
	    throw std::runtime_error("Column 'primary_procedure' must have type Varchar");
	}
	
	// Get primary diagnosis
	try {
	    auto raw{column<Varchar>("primary_diagnosis", row).read()};
	    primary_diagnosis_ = parser.parse_diagnosis(raw);
	} catch (const std::out_of_range &) {
	    throw std::runtime_error("Missing required 'primary_diagnosis' column in row");
	} catch (const std::bad_variant_access &) {
	    throw std::runtime_error("Column 'primary_diagnosis' must have type Varchar");
	}

	// Get secondary procedures -- needs refactoring, but need to fix parse_procedure/
	// parse_diagnosis first (i.e. merge them)
	for (std::size_t n{0}; true; n++) {
	    try {
		auto column_name{"secondary_procedure_" + std::to_string(n)};
		auto raw{column<Varchar>(column_name, row).read()};
		auto procedure{parser.parse_procedure(raw)};
		if (not procedure.null()) {
		    secondary_procedures_.push_back(procedure);
		} else {
		    // Found a procedure that is empty (i.e. whitespace),
		    // stop searching further columns
		    break;
		}
	    } catch (const std::out_of_range &) {
		// Reached first secondary column that does not exist, stop
		break;
	    } catch (const std::bad_variant_access &) {
		throw std::runtime_error("Column 'secondary_diagnosis_<n>' must have type Varchar");
	    }
	}

	// Get secondary diagnoses
	for (std::size_t n{0}; true; n++) {
	    try {
		auto column_name{"secondary_diagnosis_" + std::to_string(n)};
		auto raw{column<Varchar>(column_name, row).read()};
		auto diagnosis{parser.parse_diagnosis(raw)};
		if (not diagnosis.null()) {
		    secondary_diagnoses_.push_back(diagnosis);
		} else {
		    // Found a diagnosis that is empty (i.e. whitespace),
		    // stop searching further columns
		    break;
		}
	    } catch (const std::out_of_range &) {
		// Reached first secondary column that does not exist, stop
		break;
	    } catch (const std::bad_variant_access &) {
		throw std::runtime_error("Column 'secondary_diagnosis_<n>' must have type Varchar");
	    }
	}
    }

    void set_primary_procedure(const ClinicalCode & clinical_code) {
	primary_procedure_ = clinical_code;
    }

    void set_primary_diagnosis(const ClinicalCode & clinical_code) {
	primary_diagnosis_ = clinical_code;
    }

    void push_secondary_procedure(const ClinicalCode & clinical_code) {
	secondary_procedures_.push_back(clinical_code);
    }

    void push_secondary_diagnosis(const ClinicalCode & clinical_code) {
	secondary_diagnoses_.push_back(clinical_code);
    }
    
    auto primary_procedure() const {
	return primary_procedure_;
    }

    auto primary_diagnosis() const {
	return primary_diagnosis_;
    }

    /// Note that the result is ordered, and not necessarily unique
    const auto & secondary_procedures() const {
	return secondary_procedures_;
    }

    /// Note that the result is ordered, and not necessarily unique
    const auto & secondary_diagnoses() const {
	return secondary_diagnoses_;
    }

    auto episode_start() const {
	return episode_start_;
    }

    auto episode_end() const {
	return episode_start_;
    }
    
    void print(std::shared_ptr<StringLookup> lookup, std::size_t pad = 0) const {
	std::cout << std::string(pad, ' ');
	std::cout << "Episode: ";
	episode_start_.print();
	std::cout << " - ";
	episode_end_.print();
	std::cout << std::endl;
	std::cout << std::string(' ', pad);
	std::cout << std::string(pad, ' ');
        std::cout << "Primary diagnosis: ";
	primary_diagnosis_.print(lookup);
        std::cout << std::endl;
	std::cout << std::string(' ', pad);
	if (secondary_diagnoses_.size() > 0) {
	    std::cout << std::string(pad, ' ');
	    std::cout << "Secondary diagnoses: " << std::endl;
	    for (const auto & diagnosis : secondary_diagnoses_) {
		std::cout << std::string(pad, ' ') << "- ";
		diagnosis.print(lookup);
		std::cout << std::endl;
	    }
	}
	std::cout << std::string(pad, ' ');	
        std::cout << "Primary procedure: ";
	primary_procedure_.print(lookup);
        std::cout << std::endl;
	if (secondary_procedures_.size() > 0) {
	    std::cout << std::string(pad, ' ');
	    std::cout << "Secondary procedures: " << std::endl;
	    for (const auto & procedure : secondary_procedures_) {
		std::cout << std::string(pad, ' ') << "- ";
		procedure.print(lookup);
		std::cout << std::endl;
	    }
	}	
    }
    
private:
    Timestamp episode_start_;
    Timestamp episode_end_;

    ClinicalCode primary_diagnosis_;
    ClinicalCode primary_procedure_;

    // Use vector to keep the order of the secondaries.
    std::vector<ClinicalCode> secondary_procedures_;
    std::vector<ClinicalCode> secondary_diagnoses_;
};

#endif
