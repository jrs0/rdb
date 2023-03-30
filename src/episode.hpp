#ifndef EPISODE_HPP
#define EPISODE_HPP

#include "row_buffer.hpp"
#include "sql_types.hpp"
#include "category.hpp"

#include "set_utils.hpp"
#include "clinical_code.hpp"

/// Get the vector of source columns from the config file node
std::vector<std::string> source_columns(const YAML::Node & config) {
    return config["source_columns"].as<std::vector<std::string>>();
}

/// Helper function to read a set of columns of codes, parse them using
/// a specified parser, and collect the valid ones into a flat vector
/// (in the same order as the column order). A code is considered invalid
/// (and not included) if the parser throws a runtime error during
/// parsing. Look at the TopLevelcategory to see what is covered.
std::set<std::string>
merge_groups_from_columns(const std::vector<std::string> & columns,
			  const RowBuffer auto & row,
			  TopLevelCategory & parser) {
    std::set<std::string> result;
    for (const auto & column_name : columns) {
	try {
	    auto raw_code{column<Varchar>(column_name, row).read()};
	    auto groups{parser.code_groups(raw_code)};
	    result.insert(groups.begin(), groups.end());
	} catch (const NullValue & /* NULL in column */) {
	    // If a null is found, assume that all the other values
	    // are null. This assumption can be relaxed later after
	    // the code is optimised. Note also that the procedure
	    // and diagnosis columns must be ordered for this to
	    // make sense.
	    //break;
	} catch (const std::runtime_error & /* invalid or not found */) {
	    // Continue
	} catch (const std::out_of_range & e) {
	    throw std::runtime_error("Could not read from column "
				     + column_name);
	}
    }
    return result;
}

/// This class has two jobs -- keep the diagnoses and
/// procedure parsers close together; and map collections
/// of groups as defined in the codes file into "meta" groups
/// that will become the column names in the data frame.
///
/// This class owns its parser, so it will be thread safe
/// at the cost of duplicated copies of the codes tree
class CodeParser {
public:
    CodeParser(const YAML::Node & parser_config)
	: include_all_{parser_config["all"].as<bool>()},
	  procedures_{load_codes_helper(parser_config["procedures"])},
	  diagnoses_{load_codes_helper(parser_config["diagnoses"])},
	  procedure_columns_{source_columns(parser_config["procedures"])},
	  diagnosis_columns_{source_columns(parser_config["diagnoses"])}
    {
	// Open both the codes files and make the
	// categories -- also store maps that group
	// together categories into higher level groups,
	// as defined by the parser_config
    }

    /// Returns the cause of death as a group, "_all_cause"
    /// if the code is valid but is not in a group, or "_unknown"
    /// if the code fails to parse
    std::set<std::string> cause_of_death(const RowBuffer auto & row) {
	try {
	    auto raw_code{column<Varchar>("cause_of_death", row).read()};
	    // Note that the cause of death is an ICD field
	    auto groups{diagnoses_.code_groups(raw_code)};
	    if (groups.size() == 0) {
		// In this case, the code was valid but not in
		// any group
		groups.insert("_all_cause");
	    }
	    return groups;
	} catch (const NullValue & /* SQL NULL value */) {	    
	    return {"_unknown"};
	} catch (const std::runtime_error & /* invalid or not found */) {
	    return {"_unknown"};
	} catch (const std::out_of_range & e) {
	    throw std::runtime_error("Could not read from 'cause of death'");
	}
    }
    
    /// Parse all the procedure codes into a flat list, omitting
    /// any codes that are invalid.  If the
    /// "all" key is enabled, then all codes are placed into a group called
    /// "_all", which will be included in the output in addition to the other
    /// groups.
    std::set<std::string> all_procedures(const RowBuffer auto & row) {
	auto all_groups{merge_groups_from_columns(procedure_columns_,
						  row, procedures_)};
	if (include_all_) {
	    all_groups.insert("_all");
	}
	return all_groups;
    }
    
    /// Parse all the diagnosis codes
    std::set<std::string> all_diagnoses(const RowBuffer auto & row) {
	auto all_groups{merge_groups_from_columns(diagnosis_columns_,
						  row, diagnoses_)};
	if (include_all_) {
	    all_groups.insert("_all");
	}
	return all_groups;
    }
    
private:
    
    /// Whether to include _all group
    bool include_all_;

    TopLevelCategory procedures_;
    TopLevelCategory diagnoses_;

    std::vector<std::string> procedure_columns_;
    std::vector<std::string> diagnosis_columns_;

    //std::map<std::string, std::string> procedures_group_map_;
    //std::map<std::string, std::string> diagnoses_group_map_;
};

class Episode {
public:

    /// Create an episode with all empty (null) fields
    Episode() = default;
    
    /// For consistency with the other functions, this constructor
    /// will also fetch the next row after it is done (modifying
    /// the row argument)
    Episode(RowBuffer auto & row, CodeParser & code_parser) {
	// Expect a single row as argument, which will be the
	// episode. The episode may contain either a procedure
	// or a diagnosis (including secondaries), or both. The
	// episode relates to what happened under one attending
	// consultant.

	episode_start_ = column<Timestamp>("episode_start", row);
	episode_end_ = column<Timestamp>("episode_end", row);

	procedures_ = code_parser.all_procedures(row);
	diagnoses_ = code_parser.all_diagnoses(row);
        
	age_at_episode_ = column<Integer>("age_at_episode", row);
	
	row.fetch_next_row();
    }

    auto primary_procedure() const {
	return primary_procedure_;
    }

    auto primary_diagnosis() const {
	return primary_procedure_;
    }

    
    const auto & secondary_procedures() const {
	return procedures_;
    }

    const auto & secondary_diagnoses() const {
	return diagnoses_;
    }

    auto episode_start() const {
	return episode_start_;
    }

    auto episode_end() const {
	return episode_start_;
    }
    
    void print(const ClinicalCodeLookup & lookup) const {
	std::cout << "  Episode: ";
	episode_start_.print();
	std::cout << " - ";
	episode_end_.print();
	std::cout << std::endl;
	for (const auto & diagnosis : secondary_diagnoses_) {
	    diagnosis.print(lookup);
	    std::cout << std::endl;
	}
	for (const auto & procedure : secondary_procedures_) {
	    procedure.print(lookup);
	    std::cout << std::endl;
	}
    }
    
private:
    Timestamp episode_start_;
    Timestamp episode_end_;

    ClinicalCode primary_diagnosis_;
    ClinicalCode primary_procedure_;
    
    std::set<ClinicalCode> secondary_procedures_;
    std::set<ClinicalCode> dsecondary_iagnoses_;
};

#endif
