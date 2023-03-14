#ifndef YAML_HPP
#define YAML_HPP

#include <yaml-cpp/yaml.h>



void parse_child(const YAML::Node & child) {
    for (const auto & cat : child) {
	std::cout << cat["category"] << ":" << std::endl;
	if (cat["child"]) {
	    parse_child(cat["child"]);
	}
    }
}

void foo(const std::string & icd10_file ) {

    try {
	YAML::Node config = YAML::LoadFile(icd10_file);

	auto child = config["child"];
	parse_child(child);
	
    } catch(const YAML::BadFile& e) {
	throw std::runtime_error("Bad YAML file");
    } catch(const YAML::ParserException& e) {
	throw std::runtime_error("YAML parsing error");
    }
}

#endif
