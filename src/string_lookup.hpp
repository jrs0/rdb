#ifndef STRING_LOOKUP
#define STRING_LOOKUP

#include <iostream>
#include <map>

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
    
private:
    std::size_t next_free_index_{0};
    // Needs to map in both directions. To be
    // improved later.
    std::map<std::size_t, std::string> index_to_string_;
    std::map<std::string, std::size_t> string_to_index_;
};

#endif
