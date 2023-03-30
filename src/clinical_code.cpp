#include "clinical_code.hpp"

/// Get the code name
std::string ClinicalCode::name(const ClinicalCodeParser & parser) const {
    return parser.string_lookup().at(data_->name_id());
}

/// Get the code ducumentation string
std::string ClinicalCode::docs(const ClinicalCodeParser & parser) const {
    return parser.string_lookup().at(data_->docs_id());
}

/// Get the set of groups associated to this
/// code
std::set<std::string> ClinicalCode::groups(const ClinicalCodeParser & parser) const {
    std::set<std::string> groups;
    auto & string_lookup{parser.string_lookup()};
    for (const auto group_id : data_->group_ids()) {
	groups.insert(string_lookup.at(group_id));
    }
    return groups;
}

