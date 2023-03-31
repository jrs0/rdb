#include "clinical_code.hpp"

/// Get the code name
std::string ClinicalCode::name(std::shared_ptr<StringLookup> lookup) const {
    return lookup->at(data_->name_id());
}

/// Get the code ducumentation string
std::string ClinicalCode::docs(std::shared_ptr<StringLookup> lookup) const {
    return lookup->at(data_->docs_id());
}

/// Get the set of groups associated to this
/// code
std::set<std::string> ClinicalCode::groups(std::shared_ptr<StringLookup> lookup) const {
    std::set<std::string> groups;
    for (const auto group_id : data_->group_ids()) {
	groups.insert(lookup->at(group_id));
    }
    return groups;
}

std::string ClinicalCodeGroup::group(std::shared_ptr<StringLookup> lookup) const {
    return lookup->at(group_id_);
}

ClinicalCodeGroup::ClinicalCodeGroup(const std::string & group, std::shared_ptr<StringLookup> lookup) {
    group_id_ = lookup->insert_string(group);
}
