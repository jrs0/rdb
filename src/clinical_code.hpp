#ifndef CLINICAL_CODE_HPP
#define CLINICAL_CODE_HPP

#include <set>
#include <string>
#include <iostream>

class ClinicalCodeLookup {
public:
    
private:
    
};

class ClinicalCode {
public:
    /// Make a null clinical code
    ClinicalCode() = default;
    /// Create a new clinical code identified
    /// by this id
    ClinicalCode(std::size_t id)
	: id_{id} {}

    /// Get the code name
    std::string name(const ClinicalCodeLookup & lookup) const {
	return lookup.name_at(id_);
    }

    /// Get the code ducumentation string
    std::string docs(const ClinicalCodeLookup & lookup) const {
	return lookup.docs_at(id_);
    }

    /// Get the set of groups associated to this
    /// code
    std::set<std::string> groups(const ClinicalCodeLookup & lookup) const {
	return lookup.groups_at(id_);
    }

    void print(const ClinicalCodeLookup & lookup) const {
	if (null_) {
	    std::cout << "Empty";
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
    bool null_{true};
    /// The indentifier into the clinical code
    /// lookup
    std::size_t id_{0};
};



#endif

