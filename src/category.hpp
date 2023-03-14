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

// Expect a key called field_name containing a string (else throw runtime error
// if it is not present or cannot be converted). 
std::string expect_string(const YAML::Node & node, const std::string & field_name) {
    if (node[field_name]) {
	// TODO Want to throw an error here if parse fails
	return node[field_name].as<std::string>();
    } else {
	throw std::runtime_error("Missing required '" + field_name + "' in category");
    }    
}

/// The type of a category of codes
class Category {
public:
    
    Category(const YAML::Node & category) {
	// Doesn't like the init list apparently
	name_ = expect_string(category, "name");
	docs_ = expect_string(category, "docs");
	
	// The exclude key may not be present
	if (category["exclude"]) {
	    exclude_ = category["exclude"].as<std::vector<std::string>>();
	}
    }

    void print() {
	std::cout << "Category: " << name_ << std::endl;
	std::cout << "- " << docs_ << std::endl;
    }
private:
    /// The category name
    std::string name_;
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
    TopLevelCategory(const YAML::Node & top_level_category) {

	// Read the groups key
	groups_ = top_level_category["groups"].as<std::vector<std::string>>();

	// Check for correct type
	if (not top_level_category["categories"]) {
	    throw std::runtime_error("Missing required 'categories' key at top level");
	} else if (not top_level_category["categories"].IsSequence()) {
	    throw std::runtime_error("Expected sequence type for 'categories' key");
	} else {   
	    for (const auto & category : top_level_category["categories"]) {
		categories_.push_back(std::make_unique<Category>(category));
	    }
	}
    }

    void print() {
	std::cout << "TopLevelCategory:" << std::endl;
	std::cout << "Groups: " << std::endl;
	for (const auto & group : groups_) {
	    std::cout << "- " << group << std::endl;
	}
	for (const auto & category_pointer_ : categories_) {
	    category_pointer_->print();
	}
    }
private:
    /// The list of groups present in the sub-catagories
    std::vector<std::string> groups_;
    /// The list of sub-categories
    std::vector<std::unique_ptr<Category>> categories_;
};

    

#endif
