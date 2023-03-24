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
#include <random>

#include <yaml-cpp/yaml.h>

/// Select a random element from a vector
template<typename T>
const T & select_random(const std::vector<T> & in,
		std::uniform_random_bit_generator auto & gen) {
    std::uniform_int_distribution<> rnd(0, in.size()-1);
    return in[rnd(gen)];
}

/// Indexes the categories
class Index {
public:
    /// Expects node to be the index node; i.e. a single string
    /// or a sequence (vector) of length 1 or 2. A runtime error
    /// is thrown for anything else. 
    Index(const YAML::Node & category);

    /// The comparison sorts by start_ before end_. This will always
    /// put indices in ascending order, because ranges do not overlap
    /// in the codes (although there may be gaps).
    friend auto operator<=>(const Index&, const Index&) = default;
    friend bool operator==(const Index&, const Index&) = default;
    
    friend bool operator<(const std::string & code, const Index& n) {
	return code < n.start_;
    }
    
    /// This is the length of the index string. It is not whether or not their
    /// index has two components. For a one component index, the length of the 
    std::size_t size() const {
	return start_.size();
    }

    bool contains(const std::string & code) const;
    
private:
    std::string start_;
    /// Note that the end of range also includes any string
    /// whose starting end_.size() characters agree with end_.
    /// To compare, you need to truncate your string and then
    /// lexicographically compare with end_
    std::string end_;
};

/**
 * \brief Tree of categories
 *
 * Each level of the codes file is a tree with a name, documentation, and an
 * optional list of sub-categories. 
 *
 */
class Category {
public:

    /// Create a category by parsing a node in the yaml file. This
    /// parses all the sub-categories too.
    Category(const YAML::Node & category);
    
    // Return true if code is (lexicographically) contained
    // in the range specified by the index of this Cat
    bool contains(const std::string & code) const;

    /// Comparison is made with respect only to the index. All other
    /// fields are ignored (i.e. equality is valid even if name/docs are
    /// different). This comparison is used for the purpose of sorting.
    friend auto operator<=>(const Category & c1, const Category & c2) {
        return c1.index_ <=> c2.index_;
    }
    friend bool operator==(const Category & c1, const Category & c2) {
        return c1.index_ == c2.index_;
    }

    friend bool operator<(const std::string & code, const Category & c) {
	return code < c.index_;
    }
    
    void print() const;

    std::string name() const {
	return name_;
    }

    std::string docs() const {
	return docs_;
    }

    /// Get a view of the excluded groups at this level
    const std::set<std::string> & exclude() const {
	return exclude_;
    }

    /// Get a view of the sub-categories
    const std::vector<Category> & categories() const {
	return categories_;
    }
    
    /// Check if this category is a leaf node (i.e. it has no
    /// sub-categories)
    bool is_leaf() const {
	return categories_.size() == 0;
    }

    /// Get a uniformly randomly chosen code from this category
    std::string
    random_code(std::uniform_random_bit_generator auto & gen) const {
	if (categories_.size() == 0) {
	    // At a leaf node, pick this code
	    return name_;
	} else {
	    return select_random(categories_, gen).random_code(gen);
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
    std::vector<Category> categories_;
    /// The set of groups that are not present in this category
    /// or below
    std::set<std::string> exclude_;
};

/// Identifies which code property is requested in
enum class CodeProperty {
    /// The name of the code or category
    Name,
    /// The documentation string for the code or category
    Docs,
    /// The groups that contain the code
    Groups
};

template<typename ReturnType>
class CachingParser {
public:
    using Parser = std::function<ReturnType(const std::string &)>;
    CachingParser(const Parser & parser)
	: parser_{parser} {}
    ReturnType parse(const std::string & input) {
	try {
	    return cache_.at(input);
	} catch (const std::out_of_range &) {
	    // TODO -- scope issue here (same name function in scope)
	    auto result{parser_(input)};
	    cache_[input] = result;
	    return result;
	}   
    }
private:
    Parser parser_;
    std::map<std::string, ReturnType> cache_;
};

/// Remove non-alphanumeric characters from code (e.g. dots)
std::string remove_non_alphanum(const std::string & code) {
    std::string s{code};
    s.erase(std::remove_if(s.begin(), s.end(), 
			   []( auto const& c ) -> bool {
			       return !std::isalnum(c);
			   }), s.end());
    return s;
}

/// Special case top level (contains a groups key)
class TopLevelCategory {
public:
    TopLevelCategory(const YAML::Node & top_level_category);
    
    std::size_t cache_size() const {
	return code_name_cache_.size();
    }
    
    void print() const;
    
    /// Parse a raw code. Return the standard name of the code,
    /// or the docs of the code if the bool flag is true.
    /// Throw a runtime error if the code is invalid or not found.
    /// Query results are cached and used to speed up the next
    /// call to the function.
    template<CodeProperty P>
    std::string code_prop(const std::string & code) {
	
	// Cover two common cases of invalid codes here
	if (std::ranges::all_of(code, isspace)) {
	    throw std::runtime_error("Code is empty");
	}
	if (code == "NULL") {
	    throw std::runtime_error("Code is NULL");
	}

	/// Strip alphanumeric for the parser
	auto code_alphanum{remove_non_alphanum(code)};

	if constexpr (P == CodeProperty::Name) {
	    return name_parser_.parse(code_alphanum);
	} else {
	    return docs_parser_.parse(code_alphanum);
	}
    }

    /// Get the set of groups associated with the code, or throw a
    /// runtime error if the code is invalid or not found. Results
    /// are cached
    std::set<std::string> code_groups(const std::string & code);
    
    /// Get a uniformly randomly chosen code from the tree.
    std::string
    random_code(std::uniform_random_bit_generator auto & gen) const {
	return select_random(categories_, gen).random_code(gen);
    }
    
private:
    /// The list of groups present in the sub-catagories
    std::set<std::string> groups_;
    /// The list of sub-categories
    std::vector<Category> categories_;

    /// Parses a code name and stores the result
    CachingParser<std::string> name_parser_;

    /// TODO: this is quite a bit of cache duplication
    /// potentially -- should really cache all three of the
    /// name, code and docs all at once.
    CachingParser<std::string> docs_parser_;
    CachingParser<std::set<std::string>> groups_parser_;
};


#endif
