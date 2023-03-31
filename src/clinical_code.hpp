#ifndef CLINICAL_CODE_HPP
#define CLINICAL_CODE_HPP

#include <set>
#include <string>
#include <iostream>
#include <optional>
#include <random>

#include "category.hpp"

/// A wrapper for the set of IDs that describe a code
class ClinicalCodeData {
public:
    ClinicalCodeData(const CacheEntry & cache_entry, std::shared_ptr<StringLookup> lookup) {
	name_id_ = lookup->insert_string(cache_entry.name());
	docs_id_ = lookup->insert_string(cache_entry.docs());
	for (const auto & group : cache_entry.groups()) {
	    group_ids_.insert(lookup->insert_string(group));
	}	
    }

    const auto & name_id() const {
	return name_id_;
    }

    const auto & docs_id() const {
	return docs_id_;
    }

    const auto & group_ids() const {
	return group_ids_;
    }
    
private:
    std::size_t name_id_;
    std::size_t docs_id_;
    std::set<std::size_t> group_ids_;
};

class ClinicalCodeParser;

/// Note that this class can be NULL, which is why
/// there is also ClinicalCodeData
class ClinicalCode {
public:

    struct Empty {};
    
    /// Make a null clinical code
    ClinicalCode() = default;
    
    /// Create a new clinical code identified
    /// by this id
    ClinicalCode(const ClinicalCodeData & data)
	: data_{data} {}

    /// Get the code name
    std::string name(std::shared_ptr<StringLookup> lookup) const;
    
    /// Get the code documentation string
    std::string docs(std::shared_ptr<StringLookup> lookup) const;
    
    /// Get the set of groups associated to this
    /// code
    std::set<std::string> groups(std::shared_ptr<StringLookup> lookup) const;

    const auto & group_ids() const {
	if (not data_) {
	    throw Empty{};
	} else {
	    return data_->group_ids();
	}
    }

    /// Is the clinical code empty
    auto null() const {
	return not data_.has_value();
    }
    
private:
    std::optional<ClinicalCodeData> data_;
};

/// Print a clinical code using strings from the lookup
inline void print(const ClinicalCode & code, std::shared_ptr<StringLookup> lookup) {
    if (code.null()) {
	std::cout << "Null";
    } else {
	std::cout << code.name(lookup)
		  << " (" << code.docs(lookup) << ") "
		  << " [";
	for (const auto & group : code.groups(lookup)) {
	    std::cout << group << ",";
	}
	std::cout << "]";
    }    
}

class ClinicalCodeGroup {
public:
    ClinicalCodeGroup(const std::string & group, std::shared_ptr<StringLookup> lookup);
    std::string group(std::shared_ptr<StringLookup> lookup) const;

    bool contains(const ClinicalCode & code) const {
	if (not code.null()) {
	    return false;
	} else {
	    return code.group_ids().contains(group_id_);
	}
    }
    
private:
    std::size_t group_id_;
};

inline void print(const ClinicalCodeGroup & group, std::shared_ptr<StringLookup> & lookup) {
    std::cout << group.group(lookup) << std::endl;
}

/// Deals with both procedures and diagnoses, but stores
/// all the results in the same string pool, so no ids will
/// ever accidentally overlap. This means that you should not
/// use the same group names in the procedures and codes file,
/// unless you want them to be counted as the same group here.
class ClinicalCodeParser {
public:

    ClinicalCodeParser(const std::string & procedure_codes_file,
		       const std::string & diagnosis_codes_file,
		       std::shared_ptr<StringLookup> & lookup)
	: lookup_{lookup},
	  procedure_parser_{YAML::LoadFile(procedure_codes_file)},
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
	    ClinicalCodeData clinical_code_data{cache_entry, lookup_};
	    return ClinicalCode{clinical_code_data};
	} catch (const TopLevelCategory::Empty &) {
	    // If the code is empty, return the null-clinical code
	    return ClinicalCode{};
	}
    }
	

    ClinicalCode parse_diagnosis(const std::string & raw_code) {
	try {
	    auto cache_entry{diagnosis_parser_.parse(raw_code)};
	    ClinicalCodeData clinical_code_data{cache_entry, lookup_};
	    return ClinicalCode{clinical_code_data};
	} catch (const TopLevelCategory::Empty &) {
	    // If the code is empty, return the null-clinical code
	    return ClinicalCode{};
	}
    }

    std::string random_procedure(std::uniform_random_bit_generator auto & gen) const {
	return procedure_parser_.random_code(gen);
    }

    std::string random_diagnosis(std::uniform_random_bit_generator auto & gen) const {
	return diagnosis_parser_.random_code(gen);
    }
    
private:
    std::shared_ptr<StringLookup> lookup_;
    TopLevelCategory procedure_parser_;
    TopLevelCategory diagnosis_parser_;
};

#endif

