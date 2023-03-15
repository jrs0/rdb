#include <iostream>
#include "category.hpp"

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

Index::Index(const YAML::Node & category) {
    if (category["index"]) {
	try {
	    if (category["index"].IsSequence()) {
		auto index_vec = expect_string_vector(category, "index");
		// The index must have just two components
		if (index_vec.size() != 2) {
		    throw std::runtime_error("Wrong length of 'index' key (expected length 2)");
		} else if (index_vec[0].size() != index_vec[1].size()) {
		    throw std::runtime_error("The two parts of the index (strings) must have equal length");
		}
		// If all is well, set the start and end
		start_ = index_vec[0];
		end_ = index_vec[1];
	    } else {
		// If only the start is present, set end = start
		start_ = expect_string(category, "index");
		end_ = start_;
	    }
	} catch (const YAML::Exception & e) {
	    throw std::runtime_error("Failed to parse 'index' key in category: " + std::string{e.what()});
	}
    } else {
	throw std::runtime_error("Missing required 'index' key in category");
    }

}

bool Index::contains(const std::string & code) const {
    // Truncates to the length of the start_ and end_
    // (they are the same length). 
    std::string trunc{code.substr(0, size())};
    // To be in the range, the truncated code must be
    // lexicographically within the range of start_ and end_.
    return (code >= start_) && (trunc <= end_);
}
 
/// Get the vector of sub categories
std::vector<Category> make_sub_categories(const YAML::Node & category) {
    if (category["categories"]) {
	if (not category["categories"].IsSequence()) {
	    throw std::runtime_error("Expected sequence type for 'categories' key");
	} else {
	    std::vector<Category> categories;
	    for (const auto & category : category["categories"]) {
		categories.emplace_back(category);
	    }
	    // It is important that the categories are sorted by index
	    // for the binary search.
	    std::ranges::sort(categories);
	    return categories;
	}
    } else {
	// There are no categories
	return std::vector<Category>{};
    }
}

Category::Category(const YAML::Node & category)
    : name_{expect_string(category, "name")},
      docs_{expect_string(category, "docs")},
      index_{category}, categories_{make_sub_categories(category)}
{
    // The exclude key is optional -- if there is not
    // exclude key, then all groups are included below
    // this level
    if (category["exclude"]) {
	exclude_ = expect_string_set(category, "exclude");
    }
}

bool Category::contains(const std::string & code) const {
    return index_.contains(code);
}

void Category::print() const {
    std::cout << "Category: " << name_ << std::endl;
    std::cout << "- " << docs_ << std::endl;
    for (const auto & category : categories_) {
	category.print();
    }
}


/// 
const Category &
locate_code_in_categories(const std::string & code,
			  const std::vector<Category> & categories) {
    
    // Look through the index keys at the current level
    // and find the position of the code. Inside the codes
    // structure, the index keys provide an array to search
    // (using binary search) for the ICD code in str.
    auto position = std::upper_bound(categories.begin(), categories.end(), code);
    const bool found = (position != std::begin(categories)) &&
	((position-1)->contains(code));
	
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

    return *position;
}

TopLevelCategory::TopLevelCategory(const YAML::Node & top_level_category)
    : groups_{expect_string_set(top_level_category, "groups")}
{	
    if (not top_level_category["categories"]) {
	throw std::runtime_error("Missing required 'categories' key at top level");
    } else {
	categories_ = make_sub_categories(top_level_category);
    }
}

void TopLevelCategory::print() const {
    std::cout << "TopLevelCategory:" << std::endl;
    std::cout << "Groups: " << std::endl;
    for (const auto & group : groups_) {
	std::cout << "- " << group << std::endl;
    }
    for (const auto & category : categories_) {
	category.print();
    }
}

std::string TopLevelCategory::parse_code(const std::string & code) {

    // Check for the empty string
    if(std::ranges::all_of(code, isspace)) {
	throw std::runtime_error("Got the empty string in parse_code()");
    }

    auto cat{locate_code_in_categories(code, categories_)};
    
    std::string result{cat.name()};
        
    return result;
	
}
    
