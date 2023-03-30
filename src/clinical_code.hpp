#ifndef CLINICAL_CODE_HPP
#define CLINICAL_CODE_HPP

#include <set>
#include <string>
#include <iostream>
#include <optional>

#include "category.hpp"

/// A wrapper for the set of IDs that describe a code
class ClinicalCodeData {
public:
    ClinicalCodeData(const CacheEntry & cache_entry, StringLookup & lookup) {
	name_id_ = lookup.insert_string(cache_entry.name());
	docs_id_ = lookup.insert_string(cache_entry.name());
	for (const auto & group : cache_entry.groups()) {
	    group_ids_.insert(lookup.insert_string(group));
	}	
    }

    auto name_id() const {
	return name_id_;
    }

    auto docs_id() const {
	return name_id_;
    }

    auto group_ids() const {
	return name_id_;
    }
    
private:
    std::size_t name_id_;
    std::size_t docs_id_;
    std::set<std::size_t> group_ids_;
};

class ClinicalCodeParser;

class ClinicalCode {
public:
    /// Make a null clinical code
    ClinicalCode() = default;
    
    /// Create a new clinical code identified
    /// by this id
    ClinicalCode(const ClinicalCodeData & data)
	: data_{data} {}

    /// Get the code name
    std::string name(const ClinicalCodeParser & lookup) const {
	return lookup.name_at(data_.name_id());
    }

    /// Get the code ducumentation string
    std::string docs(const ClinicalCodeParser & lookup) const {
	return lookup.docs_at(data_.docs_id());
    }

    /// Get the set of groups associated to this
    /// code
    std::set<std::string> groups(const ClinicalCodeParser & lookup) const {
	for (const auto group_id : data_.) {
	    statements
	}
	return lookup.groups_at(id_);
    }

    /// Is the clinical code empty
    auto null() const {
	return static_cast<bool>(data_);
    }
    
    
    void print(const ClinicalCodeParser & lookup) const {
	if (null()) {
	    std::cout << "Null";
	} else {
	    std::cout << name(lookup)
		      << " (" << docs(lookup) << ") "
		      << " [";
	    for (const auto & group : groups(lookup)) {
		std::cout << group << ",";
	    }
	}
    }
    
private:
    std::optional<ClinicalCodeData> data_;
};

class ClinicalCodeParser {
public:

    /// Parse a raw code string and return the clinical code
    /// that results. Parsing results are cached, and the
    /// code name, docs and group strings are stored in a pool
    /// inside this object with an id stored in the returned
    /// object. 
    ClinicalCode parse(const std::string & raw_code) {
	auto cache_entry{parser_.parse(raw_code)};
	ClinicalCodeData clinical_code_data{cache_entry, lookup_};
	
    }
private:
    StringLookup lookup_;
    TopLevelCategory parser_;
};

#endif

