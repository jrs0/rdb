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
		    throw std::runtime_error("Wrong length of 'index' key (expected length 2) at "  + category["name"].as<std::string>());
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

/// Return the category in the supplied vector that contains the code,
/// or throw a runtime_error if the code is not found anywhere.
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
	throw std::runtime_error("Code not found in any category");
    }

    // Decrement the position to point to the largest category
    // c such that c <= code
    position--;
    
    return *position;
}

/// Return the name or docs field of a code (depending on the bool argument)
/// if it exists in the categories tree, or throw a runtime error for an invalid code
std::string get_code_prop(const std::string code,
			  const std::vector<Category> & categories,
			  bool docs,
			  std::set<std::string> groups = std::set<std::string>{}) {
    
    // Locate the category containing the code at the current level
    auto cat{locate_code_in_categories(code, categories)};

    // Check for any group exclusions at this level and remove
    // them from the current group list (note that if exclude
    // is not present, NULL is returned, which works fine).
    for (const auto & excluded_group : cat.exclude()) {
	groups.erase(excluded_group);
    }

    // If there is a subcategory, make a call to this function
    // to process the next category down. Otherwise you are
    // at a leaf node, so start returning up the call graph.
    // TODO: since this function is linearly recursive,
    // there should be a tail-call optimisation available here
    // somewhere.
    if (not cat.is_leaf()) {
	// There are sub-categories -- parse the code at the next level
	// down (put a try catch here for the case where the next level
	// down isn't better)
	return get_code_prop(code, cat.categories(), docs, groups);
    } else {
	if (docs) {
	    return cat.docs();
	} else {
	    return cat.name();
	}
    }
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

/// Remove non-alphanumeric characters from code (e.g. dots)
std::string remove_non_alphanum(const std::string & code) {
    std::string s{code};
    s.erase(std::remove_if(s.begin(), s.end(), 
			   []( auto const& c ) -> bool {
			       return !std::isalnum(c);
			   }), s.end());
    return s;
}

std::string TopLevelCategory::get_code_prop(const std::string & code, bool docs) {

    // Check for the empty string
    if(std::ranges::all_of(code, isspace)) {
	return "[EMPTY]";
    }

    auto code_alphanum{remove_non_alphanum(code)};

    // Inspect the cache.
    //
    // For procedure codes (OPCS), there are about 1800 unique
    // codes in 50,000; 2400 in 100,000; 3100 in 200,000; 3800 in 400,000.
    // 400,000 rows takes about 180 seconds.
    //
    // For diagnosis codes (ICD), there are about 90 unique codes in
    // 50,000; 300 codes in 100,000; 600 codes in 200,000; 1100 codes
    // in 400,000. 400,000 rows takes about 6 seconds. 
    try {
	return code_name_cache_.at(code_alphanum);
    } catch (const std::out_of_range &) {
	// TODO -- scope issue here (same name function in scope)
	auto code_name{::get_code_prop(code_alphanum, categories_, docs)};
	code_name_cache_.insert({code_alphanum, code_name});
	return code_name;
    }   
}
    
