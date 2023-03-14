/**
 * \file category.hpp
 *
 * This file contains the Category class, which is the basis for storing
 * the tree of code categories for ICD codes.
 *
 */

#ifndef CATEGORY_HPP
#define CATEGORY_HPP

#include <yaml-cpp/yaml.h>

/// The type of a category of codes
class Category {
public:
private:
    /// The category name
    std::string category_;
    /// The category description
    std::string docs_;
    /// The list of groups that are excluded below this level
    std::vector<std::string> exclude_;
    /// The list of subcategories within this category.
    /// If there are no subcategories, then this list is empty
    std::vector<std::unique_ptr<Category>> sub_cats_;
    /// An index used to order a collection of categories
    std::string index_;
};

/// Special case top level (contains a groups key)
class TopLevelCategory {
public:
    TopLevelCategory(const std::string & file_name) {
	try {
	    YAML::Node top_level_category = YAML::LoadFile(file_name);

	    // Read the groups key
	    groups_ = top_level_category["groups"].as<std::vector<std::string>>();

	    // Read the 
	    
	} catch(const YAML::BadFile& e) {
	    throw std::runtime_error("Bad YAML file");
	} catch(const YAML::ParserException& e) {
	    throw std::runtime_error("YAML parsing error");
	}
    }

    void print() {
	std::cout << "TopLevelCategory:" << std::endl;
	std::cout << "Groups: " << std::endl;
	for (const auto & group : groups_) {
	    std::cout << "- " << group << std::endl;
	}
    }
private:
    /// The list of groups present in the sub-catagories
    std::vector<std::string> groups_;
    /// The list of sub-categories
    std::vector<std::unique_ptr<Category>> sub_cats_;
};

    

#endif
