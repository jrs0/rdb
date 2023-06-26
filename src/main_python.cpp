#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <fstream>
#include <optional>

#include "acs.h"
#include "clinical_code.h"
#include "config.h"
#include "patient.h"
#include "sql_connection.h"
#include "sql_query.h"
#include "string_lookup.h"
#include "table.h"

/// Writes a timestamp object to a YAML stream. Includes the unix timestamp
/// with the "timestamp" key, and a human-readable string with the "readable"
/// key. Does not modify the stream if the Timestamp is NULL
YAML::Emitter &operator<<(YAML::Emitter &ys, const Timestamp &timestamp) {
    if (not timestamp.null()) {
        ys << YAML::BeginMap
           << YAML::Key << "timestamp"
           << YAML::Value << timestamp.read();
        std::stringstream ss;
        ss << timestamp;
        ys << YAML::Key << "readable"
           << YAML::Value << ss.str()
           << YAML::EndMap;
    }
    return ys;
}

/// Writes an Integer to an YAML stream. If the integer is null, no operation is
/// performed on the stream
YAML::Emitter &operator<<(YAML::Emitter &ys, const Integer &value) {
    if (not value.null()) {
        ys << value.read();
    }
    return ys;
}

void write_event_count(YAML::Emitter &ys,
                       const std::map<ClinicalCodeGroup, std::size_t> &counts,
                       std::shared_ptr<StringLookup> lookup) {
    ys << YAML::BeginSeq;
    for (const auto &[group, count] : counts) {
        ys << YAML::BeginMap
           << YAML::Key << "name"
           << YAML::Value << group.name(lookup)
           << YAML::Key << "count"
           << YAML::Value << count
           << YAML::EndMap;
    }
    ys << YAML::EndSeq;
}

/// Write a ClinicalCode to a YAML stream. Not a normal operator << because of
/// the need for the lookup. If the code is null, then nothing is printed to
/// the stream. Otherwise, a "name" and "docs" field is added for the code. In
/// addition, if the code is present in any groups, these are included in an
/// optional groups field. If the code is invalid, the docs field contains the
/// string "Unknown".
///
void write_yaml_stream(YAML::Emitter &ys, const EventCounter &event_counter,
                       std::shared_ptr<StringLookup> lookup) {
    ys << YAML::BeginMap;
    if (not event_counter.counts_before().empty()) {
        ys << YAML::Key << "before"
           << YAML::Value;
        write_event_count(ys, event_counter.counts_before(), lookup);
    }

    if (not event_counter.counts_after().empty()) {
        ys << YAML::Key << "after"
           << YAML::Value;
        write_event_count(ys, event_counter.counts_after(), lookup);
    }
    ys << YAML::EndMap;
}

/// Write a ClinicalCode to a YAML stream. Not a normal operator << because of
/// the need for the lookup. If the code is null, then nothing is printed to
/// the stream. Otherwise, a "name" and "docs" field is added for the code. In
/// addition, if the code is present in any groups, these are included in an
/// optional groups field. If the code is invalid, the docs field contains the
/// string "Unknown".
///
void write_yaml_stream(YAML::Emitter &ys, const ClinicalCode &code,
                       std::shared_ptr<StringLookup> lookup) {
    if (not code.null()) {
        ys << YAML::BeginMap;
        ys << YAML::Key << "name"
           << YAML::Value << code.name(lookup);
        ys << YAML::Key << "docs";
        if (code.valid()) {
            ys << YAML::Value << code.docs(lookup);
        } else {
            ys << YAML::Value << "Unknown";
        }
        auto code_groups{code.groups()};
        if (not code_groups.empty()) {
            ys << YAML::Key << "groups"
               << YAML::Value
               << YAML::BeginSeq;
            for (const auto &group : code_groups) {
                ys << group.name(lookup);
            }
            ys << YAML::EndSeq;
        }
        ys << YAML::EndMap;
    }
}

/// Write the mortality data to a YAML map. Contains the compulsory key "alive",
/// which is true unless there is mortality data. If there is mortality data,
/// it consists of the keys "date_of_death" (a timestamp) and "cause_of_death"
/// (a clinical code).
void write_yaml_stream(YAML::Emitter &ys, const Mortality &mortality,
                       std::shared_ptr<StringLookup> lookup) {
    ys << YAML::BeginMap;
    ys << YAML::Key << "alive"
       << YAML::Value << mortality.alive();

    if (not mortality.alive()) {
        if (not mortality.date_of_death().null()) {
            ys << YAML::Key << "date_of_death"
               << YAML::Value << mortality.date_of_death();
        }
        if (mortality.cause_of_death().has_value()) {
            ys << YAML::Key << "cause_of_death";
            ys << YAML::Value;
            write_yaml_stream(ys, mortality.cause_of_death().value(), lookup);
        }
        if (not mortality.age_at_death().null()) {
            ys << YAML::Key << "age_at_death";
            ys << YAML::Value << mortality.age_at_death();
        }
    }
    ys << YAML::EndMap;
}

/// Write an episode to a yaml stream.
void write_yaml_stream(YAML::Emitter &ys, const Episode &episode,
                       std::shared_ptr<StringLookup> lookup) {
    ys << YAML::BeginMap
       << YAML::Key << "start_date"
       << YAML::Value << episode.episode_start()
       << YAML::Key << "end_date"
       << YAML::Value << episode.episode_end();

    if (not episode.primary_diagnosis().null()) {
        ys << YAML::Key << "primary_diagnosis"
           << YAML::Value;
        write_yaml_stream(ys, episode.primary_diagnosis(), lookup);
    }

    if (not episode.primary_procedure().null()) {
        ys << YAML::Key << "primary_procedure"
           << YAML::Value;
        write_yaml_stream(ys, episode.primary_procedure(), lookup);
    }

    if (episode.secondary_diagnoses().size() > 0) {
        ys << YAML::Key << "secondary_diagnoses"
           << YAML::Value
           << YAML::BeginSeq;
        for (const auto &diagnosis : episode.secondary_diagnoses()) {
            write_yaml_stream(ys, diagnosis, lookup);
        }
        ys << YAML::EndSeq;
    }

    if (episode.secondary_procedures().size() > 0) {
        ys << YAML::Key << "secondary_procedures"
           << YAML::Value
           << YAML::BeginSeq;
        for (const auto &diagnosis : episode.secondary_procedures()) {
            write_yaml_stream(ys, diagnosis, lookup);
        }
        ys << YAML::EndSeq;
    }
    ys << YAML::EndMap;
}

/// Write a spell to a yaml stream.
void write_yaml_stream(YAML::Emitter &ys, const Spell &spell,
                       std::shared_ptr<StringLookup> lookup) {
    ys << YAML::BeginMap
       << YAML::Key << "id"
       << YAML::Value << spell.id()
       << YAML::Key << "start_date"
       << YAML::Value << spell.start_date()
       << YAML::Key << "end_date"
       << YAML::Value << spell.end_date();

    if (spell.episodes().size() > 0) {
        ys << YAML::Key << "episodes"
           << YAML::Value
           << YAML::BeginSeq;
        for (const auto &episode : spell.episodes()) {
            write_yaml_stream(ys, episode, lookup);
        }
        ys << YAML::EndSeq;
    }
    ys << YAML::EndMap;
}

void print_sql_query(const std::string &config_path) {
    // std::string config_path_str{Rcpp::as<std::string>(config_path)};
    try {
        auto config{load_config_file(config_path)};
        auto sql_query{make_acs_sql_query(config["sql_query"], true, std::nullopt)};
        std::cout << sql_query << std::endl;
    } catch (const std::runtime_error &e) {
        std::cout << "Failed with error: " << e.what() << std::endl;
    }
}

/// @brief Return a vector of predictors for bleeding,
/// @param config_path Path to the config file (e.g. "scripts/config.yaml")
/// @return A map from names to numerical values. All the values ending with _before
/// are predictors, and the "bleeding" column is the outcome
std::map<std::string, std::vector<long long>> make_acs_dataset(const std::string &config_path) {
    try {
        auto lookup{new_string_lookup()};
        auto config{load_config_file(config_path)};
        auto parser{new_clinical_code_parser(config["parser"], lookup)};
        auto sql_connection{new_sql_connection(config["connection"])};
        auto nhs_number_filter{std::nullopt};
        auto with_mortality{true};
        auto sql_query{make_acs_sql_query(config["sql_query"], with_mortality, nhs_number_filter)};

        ClinicalCodeMetagroup acs_metagroup{config["code_groups"]["acs"], lookup};
        ClinicalCodeMetagroup pci_metagroup{config["code_groups"]["pci"], lookup};
        ClinicalCodeMetagroup cardiac_death_metagroup{config["code_groups"]["cardiac_death"], lookup};
        ClinicalCodeMetagroup stemi_metagroup{config["code_groups"]["stemi"], lookup};

        std::cout << "Executing query" << std::endl;
        auto row{sql_connection.execute_direct(sql_query)};

        auto save_records{config["save_records"].as<bool>()};

        std::cout << "Started fetching rows" << std::endl;

        // Includes bleeding outcome counts, prior-event counts, age, stemi/nstemi
        // (1 for stemi, 0 for nstemi), and pci/acs triggered inclusion (1 for pci,
        // 0 for acs).
        std::map<std::string, std::vector<long long>> numerical_results;

        std::vector<std::string> nhs_numbers;
        std::vector<long long> index_dates;
        // std::vector<std::string> index_types;
        // std::vector<int> ages_at_index;
        // std::vector<std::string> stemi_presentations;
        std::vector<int> survival_times;
        std::vector<std::string> causes_of_death;

        // unsigned cancel_counter{0};
        // unsigned ctrl_c_counter_limit{10};
        const auto all_groups{parser->all_groups(lookup)};

        std::ofstream patient_records_file{"gendata/records.yaml"};
        patient_records_file << "# Each item in this list is an ACS/PCI record" << std::endl;

        while (true) {
            // TODO replace with pybind cancel
            /* if (++cancel_counter > ctrl_c_counter_limit)
            {
                Rcpp::checkUserInterrupt();
                cancel_counter = 0;
            } */

            try {
                Patient patient{row, parser};
                auto index_spells{get_acs_and_pci_spells(patient.spells(), acs_metagroup, pci_metagroup)};
                if (index_spells.empty()) {
                    continue;
                }

                auto row_number{row.current_row_number()};
                if (row_number % 100000 == 0) {
                    std::cout << "Got to row " << row_number << std::endl;
                }

                auto nhs_number{patient.nhs_number()};
                const auto &mortality{patient.mortality()};

                for (const auto &index_spell : index_spells) {
                    if (index_spell.empty()) {
                        continue;
                    }

                    nhs_numbers.push_back(std::to_string(nhs_number));

                    const auto &first_episode_of_index{get_first_episode(index_spell)};

                    const auto pci_triggered{primary_pci(first_episode_of_index, pci_metagroup)};
                    if (pci_triggered) {
                        numerical_results["index_type"].push_back(1);
                    } else {
                        numerical_results["index_type"].push_back(0);
                    }

                    auto age_at_index{first_episode_of_index.age_at_episode()};
                    try {
                        numerical_results["age"].push_back(age_at_index.read());
                    } catch (const Integer::Null &) {
                        numerical_results["age"].push_back(-1);
                    }

                    auto date_of_index{first_episode_of_index.episode_start()};
                    index_dates.push_back(date_of_index.read());

                    auto stemi_flag{get_stemi_presentation(index_spell, stemi_metagroup)};
                    if (stemi_flag) {
                        numerical_results["stemi"].push_back(1);
                    } else {
                        numerical_results["stemi"].push_back(0);
                    }

                    // Count events before/after
                    // Do not add secondary procedures into the counts, because they
                    // often represent the current index procedure (not prior procedures)
                    EventCounter event_counter;
                    for (const auto &group : get_index_secondary_groups(index_spell, CodeType::Diagnosis)) {
                        event_counter.push_before(group);
                    }

                    auto spells_before{get_spells_in_window(patient.spells(), index_spell, -365 * 24 * 60 * 60)};
                    for (const auto &group : get_all_groups(spells_before)) {
                        event_counter.push_before(group);
                    }

                    auto spells_after{get_spells_in_window(patient.spells(), index_spell, 365 * 24 * 60 * 60)};
                    for (const auto &group : get_all_groups(spells_after)) {
                        event_counter.push_after(group);
                    }

                    // Get the counts before and after for this record
                    auto before{event_counter.counts_before()};
                    auto after{event_counter.counts_after()};
                    for (const auto &group : all_groups) {
                        numerical_results[group.name(lookup) + "_before"].push_back(before[group]);

                        // Don't include any after counts for now.
                        // event_counts[group.name(lookup) + "_after"].push_back(after[group]);
                    }

                    // Make the bleeding group. This shoud return the same clinical code
                    // group as is present in the map key (because the lookup ids are unique)
                    const ClinicalCodeGroup bleeding_group{"bleeding", lookup};
                    numerical_results["bleeding"].push_back(after[bleeding_group]);

                    // Record mortality info
                    auto death_after{false};
                    auto cardiac_death{false};
                    std::optional<TimestampOffset> survival_time;
                    if (not mortality.alive()) {
                        auto date_of_death{mortality.date_of_death()};
                        if (not date_of_death.null() and not date_of_index.null()) {
                            if (date_of_death < date_of_index) {
                                throw std::runtime_error("Unexpected date of death before index date at patient" + std::to_string(nhs_number));
                            }

                            // Check if death occurs in window after (hardcoded for now)
                            survival_time = date_of_death - date_of_index;
                            if (survival_time.value() < years(1)) {
                                death_after = true;
                                auto cause_of_death{mortality.cause_of_death()};
                                if (cause_of_death.has_value()) {
                                    cardiac_death = cardiac_death_metagroup.contains(cause_of_death.value());
                                }
                            }
                        }
                    }
                    if (death_after) {
                        survival_times.push_back(survival_time.value().value());
                        if (cardiac_death) {
                            causes_of_death.push_back("cardiac");
                        } else {
                            causes_of_death.push_back("all_cause");
                        }
                    } else {
                        survival_times.push_back(-1);
                        causes_of_death.push_back("no_death");
                    }

                    if (save_records) {
                        std::cout << "====================================" << std::endl;
                        std::cout << "PCI/ACS RECORD" << std::endl;
                        std::cout << "------------------------------------" << std::endl;

                        // Provided the top level file is a list, it is fine (from the
                        // perspective of yaml syntax) to just join multiple files together.
                        // This avoids storing the entire YAML document in memory. Note that
                        // you need newlines between the list items. One is inserted below
                        // as insurance.
                        YAML::Emitter patient_record;
                        patient_record << YAML::BeginSeq;

                        /////////// print
                        std::cout << "Pseudo NHS Number: " << nhs_number << std::endl;
                        std::cout << "Age at index: " << age_at_index << std::endl;
                        std::cout << "Index date: " << date_of_index << std::endl;

                        if (stemi_flag) {
                            std::cout << "Presentation: STEMI" << std::endl;
                        } else {
                            std::cout << "Presentation: NSTEMI" << std::endl;
                        }

                        if (pci_triggered) {
                            std::cout << "Inclusion trigger: PCI" << std::endl;
                        } else {
                            std::cout << "Inclusion trigger: ACS" << std::endl;
                        }

                        /////////// end print

                        patient_record << YAML::BeginMap
                                       << YAML::Key << "nhs_number"
                                       << YAML::Value << nhs_number;

                        if (not age_at_index.null()) {
                            patient_record << YAML::Key << "age_at_index"
                                           << YAML::Value << age_at_index;
                        }

                        if (not date_of_index.null()) {
                            patient_record << YAML::Key << "date_of_index"
                                           << YAML::Value << date_of_index;
                        }

                        patient_record << YAML::Key << "presentation";
                        if (stemi_flag) {
                            patient_record << YAML::Value << "STEMI";
                        } else {
                            patient_record << YAML::Value << "NSTEMI";
                        }

                        patient_record << YAML::Key << "inclusion_trigger";
                        if (pci_triggered) {
                            patient_record << YAML::Value << "PCI";
                        } else {
                            patient_record << YAML::Value << "ACS";
                        }

                        patient_record << YAML::Key << "mortality";
                        write_yaml_stream(patient_record, mortality, lookup);

                        patient_record << YAML::Key << "index_spell";
                        write_yaml_stream(patient_record, index_spell, lookup);

                        if (not spells_after.empty()) {
                            patient_record << YAML::Key << "spells_after"
                                           << YAML::Value
                                           << YAML::BeginSeq;
                            for (const auto &spell : spells_after) {
                                write_yaml_stream(patient_record, spell, lookup);
                            }
                            patient_record << YAML::EndSeq;
                        }

                        if (not spells_before.empty()) {
                            patient_record << YAML::Key << "spells_before"
                                           << YAML::Value
                                           << YAML::BeginSeq;
                            for (const auto &spell : spells_before) {
                                write_yaml_stream(patient_record, spell, lookup);
                            }
                            patient_record << YAML::EndSeq;
                        }

                        patient_record << YAML::Key << "event_counts"
                                       << YAML::Value;
                        write_yaml_stream(patient_record, event_counter, lookup);

                        //////////////// end yaml

                        mortality.print(std::cout, lookup);
                        if (survival_time.has_value()) {
                            std::cout << "Survival time: " << survival_time.value() << std::endl;
                        }
                        std::cout << "EVENT COUNTS" << std::endl;
                        event_counter.print(std::cout, lookup);
                        std::cout << "INDEX SPELL" << std::endl;
                        index_spell.print(std::cout, lookup, 4);
                        std::cout << std::endl;
                        std::cout << "SPELLS AFTER" << std::endl;
                        for (const auto &spell : spells_after) {
                            spell.print(std::cout, lookup, 4);
                        }
                        std::cout << "SPELLS BEFORE" << std::endl;
                        for (const auto &spell : spells_before) {
                            spell.print(std::cout, lookup, 4);
                        }

                        patient_record << YAML::EndMap;
                        patient_record << YAML::EndSeq;
                        // Includes newline for insurance (concatenating yaml lists)
                        patient_records_file << std::endl
                                             << patient_record.c_str();
                    }
                }
            } catch (const RowBufferException::NoMoreRows &) {
                std::cout << "Finished fetching all rows" << std::endl;
                break;
            }
        }

        return numerical_results;
    } catch (const std::runtime_error &e) {
        std::cout << "Failed with error: " << e.what() << std::endl;
        return {};
    }
}

/// @brief Read all the clinical codes in the primary and secondary fields of an episode and
/// append the counts to a map
/// @param counts Map from code IDs (for a string lookup) to counts. Modified in place.
/// @param episode The spell to read from
///
void append_episode_codes_to_counts(Table &counts, const Episode &episode) {
    for (const auto &clinical_code : episode.all_procedures_and_diagnosis()) {
        if (clinical_code.valid()) {
            counts.increment_count(clinical_code.name_id());
        }
    }
}

/// @brief Read all the clinical codes in the primary and secondary fields of all the episodes in
/// a spell and append counts to a map
/// @param counts Map from code IDs (for a string lookup) to counts. Modified in place.
/// @param spell The spell to read from
///
///
void append_spell_codes_to_counts(Table &counts, const Spell &spell) {
    for (const auto &episode : spell.episodes()) {
        append_episode_codes_to_counts(counts, episode);
    }
}

/// @brief Return a dataset for subsequent bleeding, which a count of all the
/// ICD codes that occurred in the previou 12 months. Each ICD code gets a columns,
/// and the columns contains the count of how many times that code occurred.
/// @param config_path Path to the config file (e.g. "scripts/config.yaml")
/// @return A map from names to numerical values. All the values ending with _before
/// are predictors, and the "bleeding" column is the outcome
std::map<std::string, std::vector<long long>> all_icd_codes(const std::string &config_path) {
    try {
        auto lookup{new_string_lookup()};
        auto config{load_config_file(config_path)};
        auto parser{new_clinical_code_parser(config["parser"], lookup)};
        auto sql_connection{new_sql_connection(config["connection"])};
        auto nhs_number_filter{std::nullopt};
        auto with_mortality{true};
        auto sql_query{make_acs_sql_query(config["sql_query"], with_mortality, nhs_number_filter)};

        ClinicalCodeMetagroup acs_metagroup{config["code_groups"]["acs"], lookup};
        ClinicalCodeMetagroup pci_metagroup{config["code_groups"]["pci"], lookup};
        ClinicalCodeMetagroup cardiac_death_metagroup{config["code_groups"]["cardiac_death"], lookup};
        ClinicalCodeMetagroup stemi_metagroup{config["code_groups"]["stemi"], lookup};

        std::cout << "Executing query" << std::endl;
        auto row{sql_connection.execute_direct(sql_query)};

        std::cout << "Started fetching rows" << std::endl;

        // Map from code ids (i.e. string lookup ids) to number of occurances of this
        // code in the spells before the index event (and in the secondary diagnoses
        // of the index event).
        Table results;

        // unsigned cancel_counter{0};
        // unsigned ctrl_c_counter_limit{10};
        const auto all_groups{parser->all_groups(lookup)};

        std::ofstream patient_records_file{"gendata/records.yaml"};
        patient_records_file << "# Each item in this list is an ACS/PCI record" << std::endl;

        while (true) {
            try {
                Patient patient{row, parser};
                auto index_spells{get_acs_and_pci_spells(patient.spells(), acs_metagroup, pci_metagroup)};
                if (index_spells.empty()) {
                    continue;
                }

                auto row_number{row.current_row_number()};
                if (row_number % 100000 == 0) {
                    std::cout << "Got to row " << row_number << std::endl;
                }

                for (const auto &index_spell : index_spells) {
                    if (index_spell.empty()) {
                        continue;
                    }

                    // Must be called before any increment/set operations for
                    // this row of data. Initialises row to zero.
                    results.add_row();

                    const auto &first_episode_of_index{get_first_episode(index_spell)};

                    const auto pci_triggered{primary_pci(first_episode_of_index, pci_metagroup)};
                    const auto pci_medman_id{lookup->insert_string("pci_medman")};
                    if (pci_triggered) {
                        results.increment_count(pci_medman_id);
                    }

                    auto age_at_index{first_episode_of_index.age_at_episode()};
                    const auto age_id{lookup->insert_string("age")};
                    try {
                        results.set(age_id, age_at_index.read());
                    } catch (const Integer::Null &) {
                        results.set(age_id, -1);
                    }

                    auto stemi_flag{get_stemi_presentation(index_spell, stemi_metagroup)};
                    const auto stemi_id{lookup->insert_string("stemi")};
                    if (stemi_flag) {
                        results.increment_count(stemi_id);
                    }

                    // Count events before/after
                    // Do not add secondary procedures into the counts, because they
                    // often represent the current index procedure (not prior procedures)
                    append_spell_codes_to_counts(results, index_spell);

                    auto spells_before{get_spells_in_window(patient.spells(), index_spell, -365 * 24 * 60 * 60)};
                    for (const auto &spell : spells_before) {
                        append_spell_codes_to_counts(results, spell);
                    }

                    const ClinicalCodeGroup bleeding_group{"bleeding", lookup};
                    const auto bleeding_id{lookup->insert_string("bleeding")};
                    auto spells_after{get_spells_in_window(patient.spells(), index_spell, 365 * 24 * 60 * 60)};
                    for (const auto &group : get_all_groups(spells_after)) {
                        if (group == bleeding_group) {
                            results.increment_count(bleeding_id);
                        }
                    }
                }
            } catch (const RowBufferException::NoMoreRows &) {
                std::cout << "Finished fetching all rows" << std::endl;
                break;
            }
        }

        // Print the number of columns
        std::cout << "Total columns: " << results.columns().size() << std::endl;

        // Includes bleeding outcome counts, prior-event counts, age, stemi/nstemi
        // (1 for stemi, 0 for nstemi), and pci/acs triggered inclusion (1 for pci,
        // 0 for acs).
        std::map<std::string, std::vector<long long>> numerical_results;

        for (const auto & [column_name, column] : results.columns()) {
            numerical_results[lookup->at(column_name)] = column;
        }

        return numerical_results;
    } catch (const std::runtime_error &e) {
        std::cout << "Failed with error: " << e.what() << std::endl;
        return {};
    }
}

PYBIND11_MODULE(pbtest, m) {
    m.doc() = "pybind11 bindings for acs dataset preprocessor";

    m.def("print_sql_query", &print_sql_query, "Print the SQL query that will be used to get the underlying data");
    m.def("make_acs_dataset", &make_acs_dataset, "Get the ACS dataset");
    m.def("all_icd_codes", &all_icd_codes, "Get bleeding as a function of all ICD code predictors");
}
