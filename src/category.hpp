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
	throw std::runtime_error("Missing required vector of strings '" + field_name + "' in category");
    }        
}

class Category;

/// Create a vector of pointers to Category objects from a list of categories. An
// error is thrown if the category is not present
std::vector<std::unique_ptr<Category>>
expect_category_vector(const YAML::Node & node,
		       const std::string & field_name) {
    if (not node[field_name]) {
	throw std::runtime_error("Missing required vector of categories '" + field_name + "'.");
    } else if (not node[field_name].IsSequence()) {
	throw std::runtime_error("Expected sequence type for vector of categories key '" + field_name + "'");
    } else {
	std::vector<std::unique_ptr<Category>> categories;
	for (const auto & category : node[field_name]) {
	    categories.push_back(std::make_unique<Category>(category));
	}
	return categories;
    }
}

/// The type of a category of codes
class Category {
public:
    
    Category(const YAML::Node & category)
	: name_{expect_string(category, "name")},
	  docs_{expect_string(category, "docs")}
    {
	// The exclude key is optional -- if there is not
	// exclude key, then all groups are included below
	// this level
	if (category["exclude"]) {
	    exclude_ = expect_string_vector(category, "exclude");
	}

	// A category may or may not contain a categories key.
	// If it does not, it is a leaf node, and the recursive
	// construction ends
	if (category["categories"]) {
	    categories_ = expect_category_vector(category, "categories");
	}

    }

    void print() {
	std::cout << "Category: " << name_ << std::endl;
	std::cout << "- " << docs_ << std::endl;
	for (const auto & category_pointer_ : categories_) {
	    category_pointer_->print();
	}
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
    std::vector<std::unique_ptr<Category>> categories_;
    /// An index used to order a collection of categories
    std::string index_;
};

/// Special case top level (contains a groups key)
class TopLevelCategory {
public:
    TopLevelCategory(const YAML::Node & top_level_category) {
	groups_ = expect_string_vector(top_level_category, "groups");
	if (not top_level_category["categories"]) {
	    throw std::runtime_error("Missing required 'categories' key at top level");
	} else {
	    categories_ = expect_category_vector(top_level_category, "categories");
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
