#include "yaml.hpp"

// Expect a key called field_name containing a string (else throw runtime error
// if it is not present or cannot be converted). 
std::string expect_string(const YAML::Node & node, const std::string & field_name) {
    if (node[field_name]) {
	// TODO Want to throw an error here if parse fails
	return node[field_name].as<std::string>();
    } else {
	throw std::runtime_error("Missing required string '" + field_name + "' in category");
    }    
}

/// Expect a field called field_name which is a list of strings (else throw
/// a runtime error
std::vector<std::string> expect_string_vector(const YAML::Node & node,
						const std::string & field_name) {
    if (node[field_name]) {
	/// TODO deal with parsing errors
	return node[field_name].as<std::vector<std::string>>();
    } else {
	throw std::runtime_error("Missing required vector of strings '"
				 + field_name + "' in category");
    }        
}

/// Expect a field called field_name which is a list of strings (else throw
/// a runtime error
std::set<std::string> expect_string_set(const YAML::Node & node,
					const std::string & field_name) {
    if (node[field_name]) {
	/// TODO deal with parsing errors
	std::set<std::string> set;
	for (const auto & item : node[field_name]) {
	    set.insert(item.as<std::string>());
	}
	return set;
    } else {
	throw std::runtime_error("Missing required set of strings '"
				 + field_name + "' in category");
    }        
}
