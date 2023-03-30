#ifndef EPISODE_HPP
#define EPISODE_HPP

#include "row_buffer.hpp"
#include "sql_types.hpp"
#include "category.hpp"

#include "set_utils.hpp"
#include "clinical_code.hpp"

#include "sql_types.hpp"

// /// Get the vector of source columns from the config file node
// std::vector<std::string> source_columns(const YAML::Node & config) {
//     return config["source_columns"].as<std::vector<std::string>>();
// }

// /// Helper function to read a set of columns of codes, parse them using
// /// a specified parser, and collect the valid ones into a flat vector
// /// (in the same order as the column order). A code is considered invalid
// /// (and not included) if the parser throws a runtime error during
// /// parsing. Look at the TopLevelcategory to see what is covered.
// std::set<std::string>
// merge_groups_from_columns(const std::vector<std::string> & columns,
// 			  const RowBuffer auto & row,
// 			  TopLevelCategory & parser) {
//     std::set<std::string> result;
//     for (const auto & column_name : columns) {
// 	try {
// 	    auto raw_code{column<Varchar>(column_name, row).read()};
// 	    auto groups{parser.code_groups(raw_code)};
// 	    result.insert(groups.begin(), groups.end());
// 	} catch (const NullValue & /* NULL in column */) {
// 	    // If a null is found, assume that all the other values
// 	    // are null. This assumption can be relaxed later after
// 	    // the code is optimised. Note also that the procedure
// 	    // and diagnosis columns must be ordered for this to
// 	    // make sense.
// 	    //break;
// 	} catch (const std::runtime_error & /* invalid or not found */) {
// 	    // Continue
// 	} catch (const std::out_of_range & e) {
// 	    throw std::runtime_error("Could not read from column "
// 				     + column_name);
// 	}
//     }
//     return result;
// }

// class CodeParser {
// public:
//     CodeParser(const YAML::Node & parser_config)
// 	: include_all_{parser_config["all"].as<bool>()},
// 	  procedures_{load_codes_helper(parser_config["procedures"])},
// 	  diagnoses_{load_codes_helper(parser_config["diagnoses"])},
// 	  procedure_columns_{source_columns(parser_config["procedures"])},
// 	  diagnosis_columns_{source_columns(parser_config["diagnoses"])}
//     {
// 	// Open both the codes files and make the
// 	// categories -- also store maps that group
// 	// together categories into higher level groups,
// 	// as defined by the parser_config
//     }

//     /// Returns the cause of death as a group, "_all_cause"
//     /// if the code is valid but is not in a group, or "_unknown"
//     /// if the code fails to parse
//     std::set<std::string> cause_of_death(const RowBuffer auto & row) {
// 	try {
// 	    auto raw_code{column<Varchar>("cause_of_death", row).read()};
// 	    // Note that the cause of death is an ICD field
// 	    auto groups{diagnoses_.code_groups(raw_code)};
// 	    if (groups.size() == 0) {
// 		// In this case, the code was valid but not in
// 		// any group
// 		groups.insert("_all_cause");
// 	    }
// 	    return groups;
// 	} catch (const NullValue & /* SQL NULL value */) {	    
// 	    return {"_unknown"};
// 	} catch (const std::runtime_error & /* invalid or not found */) {
// 	    return {"_unknown"};
// 	} catch (const std::out_of_range & e) {
// 	    throw std::runtime_error("Could not read from 'cause of death'");
// 	}
//     }
    
//     /// Parse all the procedure codes into a flat list, omitting
//     /// any codes that are invalid.  If the
//     /// "all" key is enabled, then all codes are placed into a group called
//     /// "_all", which will be included in the output in addition to the other
//     /// groups.
//     std::set<std::string> all_procedures(const RowBuffer auto & row) {
// 	auto all_groups{merge_groups_from_columns(procedure_columns_,
// 						  row, procedures_)};
// 	if (include_all_) {
// 	    all_groups.insert("_all");
// 	}
// 	return all_groups;
//     }
    
//     /// Parse all the diagnosis codes
//     std::set<std::string> all_diagnoses(const RowBuffer auto & row) {
// 	auto all_groups{merge_groups_from_columns(diagnosis_columns_,
// 						  row, diagnoses_)};
// 	if (include_all_) {
// 	    all_groups.insert("_all");
// 	}
// 	return all_groups;
//     }
    
// private:
    
//     /// Whether to include _all group
//     bool include_all_;

//     TopLevelCategory procedures_;
//     TopLevelCategory diagnoses_;

//     std::vector<std::string> procedure_columns_;
//     std::vector<std::string> diagnosis_columns_;

//     //std::map<std::string, std::string> procedures_group_map_;
//     //std::map<std::string, std::string> diagnoses_group_map_;
// };

/// A mock row for testing the Episode row constructor
class EpisodeRowBuffer {
public:
    EpisodeRowBuffer() {
	columns_["episode_start"] = Timestamp{156173245};
	columns_["episode_end"] = Timestamp{156173280};
    }

    std::size_t size() const {
	return columns_.size();
    }

    void set_primary_diagnosis(const std::string & raw) {
        columns_["primary_diagnosis"] = Varchar{raw};	
    }

    void set_primary_procedure(const std::string & raw) {
        columns_["primary_procedure"] = Varchar{raw};	
    }

    void set_secondary_procedures(const std::vector<std::string> & vec) {
	for (const auto & raw : vec) {
	    push_secondary_procedure(raw);
	}
    }

    void set_secondary_diagnoses(const std::vector<std::string> & vec) {
	for (const auto & raw : vec) {
	    push_secondary_diagnosis(raw);
	}
    }
    
    void push_secondary_procedure(const std::string & raw) {
	auto column_name{"secondary_procedure_"
			 + std::to_string(num_secondary_procedures_++)};
	columns_[column_name] = Varchar{raw};
    }

    void push_secondary_diagnosis(const std::string & raw) {
	auto column_name{"secondary_diagnosis_"
			 + std::to_string(num_secondary_diagnoses_++)};
	columns_[column_name] = Varchar{raw};
    }
    
    
    /// Throws out_of_range if column does not exist, and
    /// bad_variant_access if T is not this column's type
    template<typename T>
    T at(std::string column_name) const {
        return std::get<T>(columns_.at(column_name));
    }
    
private:
    std::map<std::string, SqlType> columns_;
    std::size_t num_secondary_procedures_{0};
    std::size_t num_secondary_diagnoses_{0};
};

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
    
    void print(const ClinicalCodeParser & parser, std::size_t pad = 0) const {
	std::cout << std::string(pad, ' ');
	std::cout << "Episode: ";
	episode_start_.print();
	std::cout << " - ";
	episode_end_.print();
	std::cout << std::endl;
	std::cout << std::string(' ', pad);
	std::cout << std::string(pad, ' ');
        std::cout << "Primary diagnosis: ";
	primary_diagnosis_.print(parser);
        std::cout << std::endl;
	std::cout << std::string(' ', pad);
	if (secondary_diagnoses_.size() > 0) {
	    std::cout << std::string(pad, ' ');
	    std::cout << "Secondary diagnoses: " << std::endl;
	    for (const auto & diagnosis : secondary_diagnoses_) {
		std::cout << std::string(pad, ' ') << "- ";
		diagnosis.print(parser);
		std::cout << std::endl;
	    }
	}
	std::cout << std::string(pad, ' ');	
        std::cout << "Primary procedure: ";
	primary_procedure_.print(parser);
        std::cout << std::endl;
	if (secondary_procedures_.size() > 0) {
	    std::cout << std::string(pad, ' ');
	    std::cout << "Secondary procedures: " << std::endl;
	    for (const auto & procedure : secondary_procedures_) {
		std::cout << std::string(pad, ' ') << "- ";
		procedure.print(parser);
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
