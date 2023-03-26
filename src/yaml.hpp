#ifndef YAML_HPP
#define YAML_HPP

#include <yaml-cpp/yaml.h>

// Expect a key called field_name containing a string (else throw runtime error
// if it is not present or cannot be converted). 
std::string expect_string(const YAML::Node & node, const std::string & field_name);

/// Expect a field called field_name which is a list of strings (else throw
/// a runtime error
std::vector<std::string> expect_string_vector(const YAML::Node & node,
					      const std::string & field_name);
/// Expect a field called field_name which is a list of strings (else throw
/// a runtime error
std::set<std::string> expect_string_set(const YAML::Node & node,
					const std::string & field_name);

#endif
