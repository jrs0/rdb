#ifndef CLINICAL_CODE_HPP
#define CLINICAL_CODE_HPP

#include <set>
#include <string>
#include <iostream>
#include <optional>
#include <random>
#include <ranges>

#include "category.hpp"
#include "colours.hpp"

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

    /// Make an invalid clinical code (prints as invalid,
    /// stores the ID of the raw string)
    ClinicalCode(std::size_t invalid_string_id)
	: invalid_{invalid_string_id} {}
    
    /// Create a new clinical code identified
    /// by this id
    ClinicalCode(const ClinicalCodeData & data)
	: data_{data} {}

    /// Get the code name. Returns the raw string for an invalid code
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
	return (not data_.has_value()) and (not invalid_.has_value());
    }

    /// Is the clinical code invalid? If
    /// it is, it will still print and show
    /// the code, but there will be no
    /// description or groups
    auto invalid() const {
	return invalid_.has_value();
    }
    
private:
    std::optional<std::size_t> invalid_{std::nullopt};
    std::optional<ClinicalCodeData> data_{std::nullopt};
};

/// Print a clinical code using strings from the lookup
inline void print(const ClinicalCode & code, std::shared_ptr<StringLookup> lookup) {
    if (code.null()) {
	std::cout << "Null";
    } else if (code.invalid()) {
	std::cout << Colour::CYAN	
		  << code.name(lookup) << " (Unknown)"
		  << Colour::RESET;
    } else {
	auto code_groups{code.groups(lookup)};
	if (not code_groups.empty()) {
	    std::cout << Colour::ORANGE;
	}
	std::cout << code.name(lookup)
		  << " (" << code.docs(lookup) << ") "
		  << " [";
        for (const auto & group : code_groups) {
	    std::cout << group << ",";
	}
	std::cout << "]";
	if (not code_groups.empty()) {
	    std::cout << Colour::RESET;
	}
    }    
}

class ClinicalCodeGroup {
public:
    ClinicalCodeGroup(const std::string & group, std::shared_ptr<StringLookup> lookup);
    std::string group(std::shared_ptr<StringLookup> lookup) const;

    bool contains(const ClinicalCode & code) const {
	if (code.null()) {
	    return false;
	} else {
	    return code.group_ids().contains(group_id_);
	}
    }

    void print(std::shared_ptr<StringLookup> lookup) const {
	std::cout << lookup->at(group_id_);
    }
    
private:
    std::size_t group_id_;
};

/// A metagroup is a set of clinical code groups
class ClinicalCodeMetagroup {
public:

    /// Empty group
    ClinicalCodeMetagroup() = default;
    
    ClinicalCodeMetagroup(const YAML::Node & group_list,
			  std::shared_ptr<StringLookup> lookup) {
	for (const auto & group : group_list) {
	    groups_.emplace_back(group.as<std::string>(), lookup);
	}
    }

    void push_back(const ClinicalCodeGroup & code) {
	groups_.push_back(code);
    }

    bool contains(const ClinicalCode & code) {
	auto contains_code{[&](const auto & group) {
	    return group.contains(code);
	}};

	return std::ranges::any_of(groups_, contains_code);
    }

    void print(std::shared_ptr<StringLookup> lookup) {
	std::cout << "[";
	for (const auto & group : groups_) {
	    group.print(lookup);
	    std::cout << ",";
	}
	std::cout << "]";
    }
    
private:
    std::vector<ClinicalCodeGroup> groups_;
};

inline void print(const ClinicalCodeGroup & group, std::shared_ptr<StringLookup> & lookup) {
    std::cout << group.group(lookup) << std::endl;
}

/// Choose whether to parse a raw code (string) as a diagnosis
/// or a procedure
enum class CodeType {
    Diagnosis,
    Procedure
};

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
    ClinicalCode parse(CodeType type, const std::string & raw_code) {
	try {
	    switch (type) {
	    case CodeType::Procedure: {
		auto cache_entry{procedure_parser_.parse(raw_code)};
		ClinicalCodeData clinical_code_data{cache_entry, lookup_};
		return ClinicalCode{clinical_code_data};
	    }
	    case CodeType::Diagnosis: {
		auto cache_entry{diagnosis_parser_.parse(raw_code)};
		ClinicalCodeData clinical_code_data{cache_entry, lookup_};
		return ClinicalCode{clinical_code_data};
	    }
	    default:
		// To remove compiler warning "control reaches end of non-void function"
		throw std::runtime_error("Not expecting to get here in parse()");
	    }
	} catch (const ParserException::Empty &) {
	    // If the code is empty, return the null-clinical code
	    return ClinicalCode{};
	} catch (const ParserException::CodeNotFound &) {
	    // Store the invalid raw code in the lookup
	    auto raw_string_id{lookup_->insert_string(raw_code)};
	    // Makes an invalid code
            return ClinicalCode{raw_string_id};
	}
    }

    std::string random_code(CodeType type,
			    std::uniform_random_bit_generator auto & gen) const {
	switch (type) {
	case CodeType::Procedure:
	    return procedure_parser_.random_code(gen);
	case CodeType::Diagnosis:
	    return diagnosis_parser_.random_code(gen);
	default:
	    // To remove compiler warning "control reaches end of non-void function"
	    throw std::runtime_error("Not expecting to get here in random_code()");
	}
    }
    
private:
    std::shared_ptr<StringLookup> lookup_;
    TopLevelCategory procedure_parser_;
    TopLevelCategory diagnosis_parser_;
};

/// Make a new parser from a configuration block
std::shared_ptr<ClinicalCodeParser>
new_clinical_code_parser(const YAML::Node &config,
                         std::shared_ptr<StringLookup> lookup);

#endif

