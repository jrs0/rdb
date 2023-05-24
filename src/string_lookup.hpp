#ifndef STRING_LOOKUP
#define STRING_LOOKUP

#include <iostream>
#include <map>
#include <memory>
#include <ranges>

/**
 * \brief Map strings to unique IDs
 *
 * This class stores a bidirectional map from strings to
 * unique non-negative number identifiers. All string data
 * (such as clinical code names, groups, etc.) are manipulated
 * using the unique ID. This lookup is then used to convert back
 * to the string when required
 */
class StringLookup {
public:
    /// Get the index of the string passed as argument, or
    /// insert the string and return the new index
    std::size_t insert_string(const std::string & string) {
	try {
	    return string_to_index_.at(string);
	} catch (const std::out_of_range &) {
	    string_to_index_[string] = next_free_index_;
	    index_to_string_[next_free_index_] = string;
	    next_free_index_++;
	    return next_free_index_ - 1;
	}
    }

    /// Get the string at the index passed as the argument, or
    /// throw out_of_range if not found
    std::string at(std::size_t index) const {
	return index_to_string_.at(index); 
    }

    void print(const std::ostream & os = std::cout) const {
	os << "String lookup:" << std::endl;
	for (const auto & [index, string] : index_to_string_) {
	    os << index << ": " << string << std::endl;
	}
    }

    /// All the strings in the lookup, in order of index
    auto strings() const {
	return index_to_string_ | std::views::values;
    }
    
private:
    std::size_t next_free_index_{0};
    // Needs to map in both directions. To be
    // improved later.
    std::map<std::size_t, std::string> index_to_string_;
    std::map<std::string, std::size_t> string_to_index_;
};

std::shared_ptr<StringLookup> new_string_lookup();

#endif
