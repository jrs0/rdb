/**
 * \file category.hpp
 *
 * This file contains the Category class, which is the basis for storing
 * the tree of code categories for ICD codes.
 *
 */

#ifndef CATEGORY_HPP
#define CATEGORY_HPP

#include <algorithm>
#include <set>

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
	throw std::runtime_error("Missing required vector of strings '" + field_name + "' in category");
    }        
}

/// Indexes the categories
class Index {
public:
    /// Expects node to be the index node; i.e. a single string
    /// or a sequence (vector) of length 1 or 2. A runtime error
    /// is thrown for anything else
    Index(const YAML::Node & category) {
	if (category["index"]) {
	    try {
		if (category["index"].IsSequence()) {
		    index_ = expect_string_vector(category, "index");
		    // The index must have just two components
		    if (index_.size() != 2) {
		 	throw std::runtime_error("Wrong length of 'index' key (expected length 2)");
		    } else if (index_[0].size() != index_[1].size()) {
			throw std::runtime_error("The two parts of the index (strings) must have equal length");
		    }
		} else {
		    index_.push_back(expect_string(category, "index"));
		}
	    } catch (const YAML::Exception & e) {
		throw std::runtime_error("Failed to parse 'index' key in category: " + std::string{e.what()});
	    }
	} else {
	    throw std::runtime_error("Missing required 'index' key in category");
	}

    }

    friend bool operator< (const Index & n1, const Index & n2) {
	return n1.index_[0] < n2.index_[0];
    }

    /// This is the length of the index string. It is not whether or not their
    /// index has two components. For a one component index, the length of the 
    std::size_t size() const {
	return index_[0].size();
    }

    bool contains(const std::string & code) const {
	// Truncates to the length of the first part of
	// the range, assuming the two parts are equal length
	std::string trunc{code.substr(0, size())};
	if (index_.size() == 2) {
	    return (code >= index_[0]) && (trunc <= index_[1]);
	} else {
	    return trunc == index_[0];
	}
    }
    
private:
    std::vector<std::string> index_;
};

class Category;

class CategoryList {
public:

    using const_iterator = std::vector<std::unique_ptr<Category>>::const_iterator;
    using iterator = std::vector<std::unique_ptr<Category>>::iterator;

    /// Empty category list
    CategoryList() = default;
    /// Gets the category list (the optional 'categories' key) from a category
    CategoryList(const YAML::Node & category) {
	// A category may or may not contain a categories key.
	// If it does not, it is a leaf node, and the recursive
	// construction ends
	if (category["categories"]) {
	    if (not category["categories"].IsSequence()) {
		throw std::runtime_error("Expected sequence type for 'categories' key");
	    } else {
		for (const auto & category : category["categories"]) {
		    categories_.push_back(std::make_unique<Category>(category));
		}
		// It is important that the categories are sorted by index
		// for the binary search.
		std::ranges::sort(categories_);
	    }
	}	
    }

    std::string parse_code(const std::string & code,
			   std::set<std::string> groups = std::set<std::string>{});
    
    const_iterator begin() const {
	return categories_.begin();
    }

    const_iterator end() const {
	return categories_.end();
    }

    // iterator begin() {
    // 	return categories_.begin();
    // }

    // iterator end() {
    // 	return categories_.end();
    // }
    
private:
    std::vector<std::unique_ptr<Category>> categories_;
};

/// The type of a category of codes
class Category {
public:
    
    Category(const YAML::Node & category)
	: name_{expect_string(category, "name")},
	  docs_{expect_string(category, "docs")},
	  index_{category}, categories_{category}
    {
	// The exclude key is optional -- if there is not
	// exclude key, then all groups are included below
	// this level
	if (category["exclude"]) {
	    exclude_ = expect_string_set(category, "exclude");
	}
    }

    // Return true if code is (lexicographically) contained
    // in the range specified by the index of this Cat
    bool contains(const std::string & code) const {
	return index_.contains(code);
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
    /// An index used to order a collection of categories
    Index index_;
    /// The list of subcategories within this category.
    /// If there are no subcategories, then this list is empty
    CategoryList categories_;
    /// The set of groups that are not present in this category
    /// or below
    std::set<std::string> exclude_;
};

std::string CategoryList::parse_code(const std::string & code, std::set<std::string> groups) {

    // Look through the index keys at the current level
    // and find the position of the code. Inside the codes
    // structure, the index keys provide an array to search
    // (using binary search) for the ICD code in str.
    auto position = std::upper_bound(categories_.begin(), categories_.end(), code);
    const bool found = (position != std::begin(categories_)) &&
	((*(position-1))->contains(code));
	
    // If found == false, then a match was not found. This
    // means that the code is not a valid member of any member
    // of this level, so it is not a valid code. TODO it still
    // may be possible to return the category above as a fuzzy
    // match -- consider implementing
    if (!found) {
	throw std::runtime_error("Invalid code");
    }

    // Decrement the position to point to the largest category
    // c such that c <= code
    position--;

    // Check for any group exclusions at this level and remove
    // them from the current group list (note that if exclude
    // is not present, NULL is returned, which works fine).
    // try {
    //     std::set<std::string> exclude = position->exclude();
    //     for (const auto & e : exclude) {
    // 	groups.erase(e);
    //     }
    // } catch (const Rcpp::index_out_of_bounds &) {
    //     // No exclude tag present, no need to remove anything,
    //     // groups is still valid
    // }
	
}


/// Special case top level (contains a groups key)
class TopLevelCategory {
public:
    TopLevelCategory(const YAML::Node & top_level_category)
	: groups_{expect_string_set(top_level_category, "groups")}
    {	
	if (not top_level_category["categories"]) {
	    throw std::runtime_error("Missing required 'categories' key at top level");
	} else {
	    categories_ = CategoryList{top_level_category};
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

    /// Parse a raw code. Return the standard name of the code.
    /// Throw a runtime error if the code is invalid or not found
    std::string parse_code(const std::string & code) {

	// Check for the empty string
	if(std::ranges::all_of(code, isspace)) {
	    throw std::runtime_error("Got the empty string in parse_code()");
	}
	
	std::string result{categories_.parse_code(code)};
	
	
	return code;
	
    }
    
private:
    /// The list of groups present in the sub-catagories
    std::set<std::string> groups_;
    /// The list of sub-categories
    CategoryList categories_;
};

    

#endif
