/**
 * \file category.h
 *
 * This file contains the Category class, which is the basis for storing
 * the tree of code categories for ICD codes.
 *
 */

#ifndef CATEGORY_HPP
#define CATEGORY_HPP

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <random>
#include <set>

#include "string_lookup.h"

/// Select a random element from a vector
template <typename T>
const T &select_random(const std::vector<T> &in,
                       std::uniform_random_bit_generator auto &gen) {
    std::uniform_int_distribution<> rnd(0, in.size() - 1);
    return in[rnd(gen)];
}

/// Indexes the categories
class Index {
   public:
    /// Expects node to be the index node; i.e. a single string
    /// or a sequence (vector) of length 1 or 2. A runtime error
    /// is thrown for anything else.
    Index(const YAML::Node &category);

    /// The comparison sorts by start_ before end_. This will always
    /// put indices in ascending order, because ranges do not overlap
    /// in the codes (although there may be gaps).
    friend auto operator<=>(const Index &, const Index &) = default;
    friend bool operator==(const Index &, const Index &) = default;

    friend bool operator<(const std::string &code, const Index &n) {
        return code < n.start_;
    }

    /// This is the length of the index string. It is not whether or not their
    /// index has two components. For a one component index, the length of the
    std::size_t size() const {
        return start_.size();
    }

    bool contains(const std::string &code) const;

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
    Category(const YAML::Node &category);

    // Do not allow copies -- there is a huge tree in this class
    Category(const Category &) = delete;
    const Category &operator=(const Category &) = delete;

    // Define move operations for sort
    Category(Category &&that) = default;
    Category &operator=(Category &&) = default;

    // Return true if code is (lexicographically) contained
    // in the range specified by the index of this Cat
    bool contains(const std::string &code) const;

    /// Comparison is made with respect only to the index. All other
    /// fields are ignored (i.e. equality is valid even if name/docs are
    /// different). This comparison is used for the purpose of sorting.
    friend auto operator<=>(const Category &c1, const Category &c2) {
        return c1.index_ <=> c2.index_;
    }
    friend bool operator==(const Category &c1, const Category &c2) {
        return c1.index_ == c2.index_;
    }

    friend bool operator<(const std::string &code, const Category &c) {
        return code < c.index_;
    }

    void print(std::ostream &os) const;

    std::string name() const {
        return name_;
    }

    std::string docs() const {
        return docs_;
    }

    /// Get a view of the excluded groups at this level
    const std::set<std::string> &exclude() const {
        return exclude_;
    }

    /// Get a view of the sub-categories
    const std::vector<Category> &categories() const {
        return categories_;
    }

    /// Check if this category is a leaf node (i.e. it has no
    /// sub-categories)
    bool is_leaf() const {
        return categories_.size() == 0;
    }

    /// Get a uniformly randomly chosen code from this category
    std::string
    random_code(std::uniform_random_bit_generator auto &gen) const {
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

/// A wrapper for the set of IDs that describe a code
class CodeCacheEntry {
   public:
    /// This is a function that should not get called too often.
    /// It performs the expensive insertion of the string into the
    /// lookup, which happens the first time a string is parsed.
    /// After that, all other instances of the same code can preprocess
    /// it, look it up in the string lookup (to get the code id), and
    /// then use that id as a key into a cache to get this struct.
    CodeCacheEntry(const Category &cat, const std::set<std::string> groups,
                   std::shared_ptr<StringLookup> lookup) {
        name_id_ = lookup->insert_string(cat.name());
        docs_id_ = lookup->insert_string(cat.docs());
        for (const auto &group : groups) {
            group_ids_.insert(lookup->insert_string(group));
        }
    }

    const auto &name_id() const {
        return name_id_;
    }

    const auto &docs_id() const {
        return docs_id_;
    }

    const auto &group_ids() const {
        return group_ids_;
    }

   private:
    std::size_t name_id_;
    std::size_t docs_id_;
    std::set<std::size_t> group_ids_;
};

/// Parses a code and caches the name, docs and groups. Make sure
/// you do some preprocessing on the code before parsing it (i.e.
/// remove whitespace etc.) to reduce the cache size.
class CachingParser {
   public:
    CachingParser(std::shared_ptr<StringLookup> lookup)
        : lookup_{lookup} {}

    CodeCacheEntry parse(const std::string &code,
                         const std::vector<Category> &categories,
                         const std::set<std::string> &all_groups);
    std::size_t cache_size() const { return cache_.size(); }

   private:
    std::shared_ptr<StringLookup> lookup_;
    std::map<std::string, CodeCacheEntry> cache_;
};

/// Do some initial checks on the code (remove whitespace
/// and non-alphanumeric characters). Throw Empty for
/// an all-whitespace or empty string.
std::string preprocess(const std::string &code);

namespace ParserException {
/// Thrown if the code is whitespace or empty
struct Empty {
};

/// Thrown if the code is invalid
struct CodeNotFound {
};
}  // namespace ParserException

/// Special case top level (contains a groups key)
class TopLevelCategory {
   public:
    TopLevelCategory(const YAML::Node &top_level_category,
                     std::shared_ptr<StringLookup> lookup);

    // Do not allow copies -- there is a huge tree in this class
    TopLevelCategory(const TopLevelCategory &) = delete;
    const TopLevelCategory &operator=(const TopLevelCategory &) = delete;

    std::size_t cache_size() const {
        return caching_parser_.cache_size();
    }

    void print(std::ostream &os) const;

    /// Take a code as a string, preprocess it, and insert it into
    /// (or get it from, if it is already there) the code lookup.
    CodeCacheEntry parse(const std::string &code) {
        auto preprocessed_code{preprocess(code)};
        return caching_parser_.parse(preprocessed_code, categories_, groups_);
    }

    /// Return all groups defined in the config file
    std::set<std::string> all_groups() const {
        return groups_;
    }

    /// Obtain a (flat) list of all codes along with code
    /// documentation in the parser (i.e. in the file)
    std::vector<std::pair<std::string, std::string>>
    all_codes_and_docs() const;

    /// Return all the codes in a particular group. Throws
    /// std::runtime_error if the group does not exist.
    std::vector<std::pair<std::string, std::string>>
    codes_in_group(const std::string &group);

    /// Get a uniformly randomly chosen code from the tree.
    std::string
    random_code(std::uniform_random_bit_generator auto &gen) const {
        return select_random(categories_, gen).random_code(gen);
    }

   private:
    /// The list of groups present in the sub-catagories
    std::set<std::string> groups_;
    /// The list of sub-categories
    std::vector<Category> categories_;

    /// Parses a code name and stores the result
    CachingParser caching_parser_;
};

#endif
