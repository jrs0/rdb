#ifndef ROW_BUFFER_HPP
#define ROW_BUFFER_HPP

#include <functional>

#include "stmt_handle.hpp"
#include "yaml.hpp"
#include "category.hpp"
#include "random.hpp"

template<class T>
concept RowBuffer = requires(T t, const std::string & s) {
    t.size();
    t.at(s);
    t.column_names();
    t.fetch_next_row();   
};

/// Load a top_level_category YAML Node from file
YAML::Node load_codes_helper(const YAML::Node & config) {
    try {
	return YAML::LoadFile(config["file"].as<std::string>());
    } catch(const YAML::BadFile& e) {
	throw std::runtime_error("Bad YAML file");
    } catch(const YAML::ParserException& e) {
	throw std::runtime_error("YAML parsing error");
    }
}

/// A SQL-like results set for making mock results for testing
/// and optimisation purposes.
///
/// This object has the following columns:
///
/// - nhs_number - a unique string
/// - spell_id - a unique string
/// - diagnosisprimary_icd - a raw ICD code
/// - diagnosis1stsecondary_icd - a raw ICD code
/// - primaryprocedure_opcs - a raw OPCS code
/// - procedure2nd_opcs - a raw OPCS code
///
/// Each patient has a number of spells, each of which comprises
/// a number of episodes (one per row). The nhs_number and spell_id
/// are globally unique (i.e. two patients do not share a spell_id
/// either). The ICD and OPCS codes are drawn at random from the
/// codes files, and are valid.
///
/// The properties of the object are controlled by a special
/// in_memory section of the config file. Other properties are
/// taken from the rest of the file (e.g. paths to code definition
/// files).
///
class InMemoryRowBuffer {
public:
    InMemoryRowBuffer(const YAML::Node & config) {

	YAML::Node in_mem{config["in_memory"]};
	
	// Create a random generator
        Seed seed{in_mem["seed"].as<std::size_t>()};
	auto gen{Generator<std::size_t,0,1>(seed)};

	std::size_t num_patients{in_mem["num_patients"].as<std::size_t>()};
	
	// Make the generators number of spells and episodes
	Random<std::size_t> spells_rnd{1, 5, seed};
	Random<std::size_t> episodes_rnd{1, 7, seed};

	auto parser_config{config["parse_config"]};
	TopLevelCategory opcs4{load_codes_helper(parser_config["procedures"])};
	TopLevelCategory icd10{load_codes_helper(parser_config["diagnoses"])};
	
	// Make the patients
	for (std::size_t n{0}; n < num_patients; n++) {
	    // Make spells
	    for (std::size_t s{0}; s < spells_rnd(); s++) {
		// Make episodes
		for (std::size_t e{0}; e < episodes_rnd(); e++) {
		    // Push the episode rows
		    table_["nhs_number"].push_back(std::to_string(n));
		    table_["spell_id"].push_back(std::to_string(s));
		    table_["diagnosisprimary_icd"].push_back(icd10.random_code(gen));
		    table_["diagnosis1stsecondary_icd"].push_back(icd10.random_code(gen));
		    table_["primaryprocedure_opcs"].push_back(opcs4.random_code(gen));
		    table_["procedure2nd_opcs"].push_back(opcs4.random_code(gen));
		}
	    }
	}
	
	       
	// Make the vector of nhs numbers
	
    }

    // Get the number of columns
    std::size_t size() const {
	return table_.size();
    }
    
    std::vector<std::string> column_names() const {
	std::vector<std::string> column_names;
	for (const auto & column : table_) {
	    column_names.push_back(column.first);
	}
	return column_names;
    }

    const std::string & at(std::string column_name) const {
	return table_.at(column_name)[current_row_index_];
    }
    
    /// Fetch the next row of data into an internal state
    /// variable. Use get() to access items from the current
    /// row. This function throws a logic error if there are
    /// not more rows.
    void fetch_next_row() {
	current_row_index_++;
	if (current_row_index_ == num_rows()) {
	    throw std::logic_error("No more rows");
	}
    }

    
    
private:

    std::size_t num_rows() const {
	return table_.at("nhs_number").size();
    }
    
    std::size_t current_row_index_{0};
    std::map<std::string, std::vector<std::string>> table_;
};

/// Holds the column bindings for an in-progress query. Allows
/// rows to be fetched one at a time.
class SqlRowBuffer {
public:
    /// Make sure you only do this after executing the stam
    SqlRowBuffer(const std::shared_ptr<StmtHandle> & stmt)
	: stmt_{stmt}
    {
	// Loop over the columns (note: indexed from 1!)
	// Get the column types
	std::size_t num_columns{stmt_->num_columns()};
	
	// Get all the column names. This is where you might do
	// column name remapping.
	for (std::size_t n = 1; n <= num_columns; n++) {
	    std::string colname{stmt_->column_name(n)};
	    col_bindings_.push_back(stmt_->make_binding(n));
	}
    }

    // Get the number of columns
    std::size_t size() const {
	return col_bindings_.size();
    }

    std::vector<std::string> column_names() const {
	std::vector<std::string> names;
	for (const auto & bind : col_bindings_) {
	    names.push_back(bind.col_name());
	}
	return names;
    }

    const std::string & at(std::string column_name) const {
	return current_row_.at(column_name);
    }
    
    /// Fetch the next row of data into an internal state
    /// variable. Use get() to access items from the current
    /// row. This function throws a logic error if there are
    /// not more rows.
    void fetch_next_row() {
	if(not stmt_->fetch()) {
	    throw std::logic_error("No more rows");
	}
	for (auto & bind : col_bindings_) {
	    std::string value;
	    try {
		value = bind.read_buffer();
	    } catch (const std::logic_error &) {
		value = "NULL";
	    }
	    auto column_name{bind.col_name()};

	    // We are assuming here that the column names
	    // are not changing from one row fetch to the next.
	    // First time will insert, next will update.
	    current_row_[column_name] = value;
	}
    }
	
private:
    std::shared_ptr<StmtHandle> stmt_;
    std::vector<ColBinding> col_bindings_;
    std::map<std::string, std::string> current_row_;
};

#endif
