#include "clinical_code.h"

/// Get the code name
std::string ClinicalCode::name(std::shared_ptr<StringLookup> lookup) const {
    if (not valid()) {
	return lookup->at(*invalid_);
    } else {
	return lookup->at(data_->name_id());
    }
}

/// Get the code ducumentation string
std::string ClinicalCode::docs(std::shared_ptr<StringLookup> lookup) const {
    return lookup->at(data_->docs_id());
}

/// Get the set of groups associated to this
/// code
std::set<ClinicalCodeGroup> ClinicalCode::groups() const {
    std::set<ClinicalCodeGroup> groups;
    for (const auto group_id : data_->group_ids()) {
	groups.insert(group_id);
    }
    return groups;
}

std::string ClinicalCodeGroup::name(std::shared_ptr<StringLookup> lookup) const {
    return lookup->at(group_id_);
}

bool ClinicalCodeGroup::contains(const ClinicalCode & code) const {
    if (not code.valid()) {
	return false;
    } else {
	return code.group_ids().contains(group_id_);
    }
}


ClinicalCodeGroup::ClinicalCodeGroup(const std::string & group, std::shared_ptr<StringLookup> lookup) {
    group_id_ = lookup->insert_string(group);
}


std::shared_ptr<ClinicalCodeParser>
new_clinical_code_parser(const YAML::Node & config,
			 std::shared_ptr<StringLookup> lookup) {
    auto procedure_file{config["procedure_file"].as<std::string>()};
    auto diagnosis_file{config["diagnosis_file"].as<std::string>()};
    return std::make_shared<ClinicalCodeParser>(procedure_file, diagnosis_file, lookup);
}
