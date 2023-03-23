#ifndef MEM_ROW_BUFFER
#define MEM_ROW_BUFFER

#include <yaml-cpp/yaml.h>
#include "category.hpp"

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
        //Seed seed{in_mem["seed"].as<std::size_t>()};
	//auto gen{Generator<std::size_t,0,1>(seed)};
	std::random_device dev;
	std::mt19937 gen(dev());
	
	std::size_t num_patients{in_mem["num_patients"].as<std::size_t>()};
	
	// Make the generators number of spells and episodes
	// Random<std::size_t> spells_rnd{1, 5, seed};
	std::uniform_int_distribution<> spells_rnd(1, 5);
	
	// Random<std::size_t> episodes_rnd{1, 7, seed};
	std::uniform_int_distribution<> episodes_rnd(1, 7);

	auto parser_config{config["parser_config"]};
	TopLevelCategory opcs4{load_codes_helper(parser_config["procedures"])};
	TopLevelCategory icd10{load_codes_helper(parser_config["diagnoses"])};
	
	// Make the patients
	for (std::size_t n{0}; n < num_patients; n++) {
	    // Make spells
	    std::cout << "P " << n << std::endl;
	    for (std::size_t s{0}; s < spells_rnd(gen); s++) {
		// Make episodes
		std::cout << "S " << s << std::endl;
		for (std::size_t e{0}; e < episodes_rnd(gen); e++) {
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

#endif
