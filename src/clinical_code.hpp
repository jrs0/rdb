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
	docs_id_ = lookup.insert_string(cache_entry.docs());
	for (const auto & group : cache_entry.groups()) {
	    group_ids_.insert(lookup.insert_string(group));
	}	
    }

    auto name_id() const {
	return name_id_;
    }

    auto docs_id() const {
	return docs_id_;
    }

    auto group_ids() const {
	return group_ids_;
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
    std::string name(const ClinicalCodeParser & parser) const;
    
    /// Get the code documentation string
    std::string docs(const ClinicalCodeParser & parser) const;
    
    /// Get the set of groups associated to this
    /// code
    std::set<std::string> groups(const ClinicalCodeParser & parser) const;
    
    /// Is the clinical code empty
    auto null() const {
	return not data_.has_value();
    }
    
    void print(const ClinicalCodeParser & parser) const {
	if (null()) {
	    std::cout << "Null";
	} else {
	    std::cout << name(parser)
		      << " (" << docs(parser) << ") "
		      << " [";
	    for (const auto & group : groups(parser)) {
		std::cout << group << ",";
	    }
	    std::cout << "]";
	}
    }
    
private:
    std::optional<ClinicalCodeData> data_;
};

/// Deals with both procedures and diagnoses, but stores
/// all the results in the same string pool, so no ids will
/// ever accidentally overlap. This means that you should not
/// use the same group names in the procedures and codes file,
/// unless you want them to be counted as the same group here.
class ClinicalCodeParser {
public:

    ClinicalCodeParser(const std::string & procedure_codes_file,
		       const std::string & diagnosis_codes_file)
	: procedure_parser_{YAML::LoadFile(procedure_codes_file)},
	  diagnosis_parser_{YAML::LoadFile(diagnosis_codes_file)}
    {}
    
    /// Parse a raw code string and return the clinical code
    /// that results. Parsing results are cached, and the
    /// code name, docs and group strings are stored in a pool
    /// inside this object with an id stored in the returned
    /// object. 
    ClinicalCode parse_procedure(const std::string & raw_code) {
	try {
	    auto cache_entry{procedure_parser_.parse(raw_code)};
	    ClinicalCodeData clinical_code_data{cache_entry, string_lookup_};
	    return ClinicalCode{clinical_code_data};
	} catch (const TopLevelCategory::Empty &) {
	    // If the code is empty, return the null-clinical code
	    return ClinicalCode{};
	}
    }
	

    ClinicalCode parse_diagnosis(const std::string & raw_code) {
	try {
	    auto cache_entry{diagnosis_parser_.parse(raw_code)};
	    ClinicalCodeData clinical_code_data{cache_entry, string_lookup_};
	    return ClinicalCode{clinical_code_data};
	} catch (const TopLevelCategory::Empty &) {
	    // If the code is empty, return the null-clinical code
	    return ClinicalCode{};
	}
    }
    
    const auto & string_lookup() const {
	return string_lookup_;
    }
    
private:
    StringLookup string_lookup_;
    TopLevelCategory procedure_parser_;
    TopLevelCategory diagnosis_parser_;
};

#endif

