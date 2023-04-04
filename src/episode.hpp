#ifndef EPISODE_HPP
#define EPISODE_HPP

#include "row_buffer.hpp"
#include "category.hpp"

#include "set_utils.hpp"
#include "clinical_code.hpp"

#include "sql_types.hpp"

ClinicalCode
read_clinical_code_column(const std::string & column_name,
			  CodeType code_type, RowBuffer auto & row,
			  std::shared_ptr<ClinicalCodeParser> parser) {
    try {
	auto raw{column<Varchar>(column_name, row).read()};
	return parser->parse(code_type, raw);
    } catch (const Varchar::Null &) {
	// Column is null, record empty code
	return ClinicalCode{};
    } catch (const RowBufferException::WrongColumnType &) {
	throw std::runtime_error("Column '" + column_name + "' must have type Varchar");
    }
}

/// Read columns named prefix<n>, where <n> is a non-negative
/// number. Short-circuit on the first empty or NULL. Throw
/// runtime error for missing columns or invalid types.
std::vector<ClinicalCode>
read_secondary_columns(const std::string & prefix, CodeType code_type,
		       RowBuffer auto & row, std::shared_ptr<ClinicalCodeParser> parser) {
    std::vector<ClinicalCode> secondaries;
    for (std::size_t n{0}; true; n++) {
	auto column_name{prefix + std::to_string(n)};

	try {
	    auto secondary{read_clinical_code_column(column_name, code_type, row, parser)};
	    if (not secondary.null()) {
		secondaries.push_back(secondary);
	    } else {
		// Found a procedure that is NULL or empty (i.e. whitespace),
		// stop searching further columns
		break;
	    }
	} catch (const RowBufferException::ColumnNotFound & ) {
	    // If you get here, then the column prefix<n> was not found. This means that
	    // you have already visited all the secondary columns in the row, so break.
	    // TODO Think about whether there should be a check to distinguish this case from
	    // the missing column case.
	    break;
	}
    }
    return secondaries;
}

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
    Episode(RowBuffer auto & row, std::shared_ptr<ClinicalCodeParser> parser) {

        episode_start_ = column<Timestamp>("episode_start", row);
        episode_end_ = column<Timestamp>("episode_end", row);

	try {
	    // Get primary procedure
	    primary_procedure_ = read_clinical_code_column("primary_procedure",
							   CodeType::Procedure,
							   row, parser);
	
	    // Get primary diagnosis
	    primary_diagnosis_ = read_clinical_code_column("primary_diagnosis",
							   CodeType::Diagnosis,
							   row, parser);	
	} catch (const RowBufferException::ColumnNotFound &) {
	    throw std::runtime_error("Missing required primary diagnosis or procedure column");
	}
	    
	// Get secondary procedures -- needs refactoring, but need to fix parse_procedure/
	// parse_diagnosis first (i.e. merge them)
	secondary_procedures_ = read_secondary_columns("secondary_procedure_",
						       CodeType::Procedure,
						       row, parser);
	
	// Get secondary diagnoses
	secondary_diagnoses_ = read_secondary_columns("secondary_diagnosis_",
						      CodeType::Diagnosis,
						      row, parser);
	
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
	::print(primary_diagnosis_, lookup);
        std::cout << std::endl;
	std::cout << std::string(' ', pad);
	if (secondary_diagnoses_.size() > 0) {
	    std::cout << std::string(pad, ' ');
	    std::cout << "Secondary diagnoses: " << std::endl;
	    for (const auto & diagnosis : secondary_diagnoses_) {
		std::cout << std::string(pad, ' ') << "- ";
		::print(diagnosis, lookup);
		std::cout << std::endl;
	    }
	}
	std::cout << std::string(pad, ' ');	
        std::cout << "Primary procedure: ";
	::print(primary_procedure_, lookup);
        std::cout << std::endl;
	if (secondary_procedures_.size() > 0) {
	    std::cout << std::string(pad, ' ');
	    std::cout << "Secondary procedures: " << std::endl;
	    for (const auto & procedure : secondary_procedures_) {
		std::cout << std::string(pad, ' ') << "- ";
		::print(procedure, lookup);
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
