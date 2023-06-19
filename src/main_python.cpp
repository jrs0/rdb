#include "string_lookup.h"
#include "config.h"
#include "clinical_code.h"
#include "sql_query.h"
#include "sql_connection.h"
#include "patient.h"

#include "acs.h"
#include <fstream>

#include <optional>

#include <pybind11/pybind11.h>

/// Writes a timestamp object to a YAML stream. Includes the unix timestamp
/// with the "timestamp" key, and a human-readable string with the "readable"
/// key. Does not modify the stream if the Timestamp is NULL
YAML::Emitter & operator<<(YAML::Emitter & ys, const Timestamp & timestamp) {
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
YAML::Emitter & operator<<(YAML::Emitter & ys, const Integer & value) {
    if (not value.null()) {
	ys << value.read();
    }
    return ys;
}

void write_event_count(YAML::Emitter & ys,
		       const std::map<ClinicalCodeGroup, std::size_t> & counts,
		       std::shared_ptr<StringLookup> lookup) {
    ys << YAML::BeginSeq;
    for (const auto & [group, count] : counts) {
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
void write_yaml_stream(YAML::Emitter & ys, const EventCounter & event_counter,
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
void write_yaml_stream(YAML::Emitter & ys, const ClinicalCode & code,
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
	    for (const auto & group : code_groups) {
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
void write_yaml_stream(YAML::Emitter & ys, const Mortality & mortality,
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
void write_yaml_stream(YAML::Emitter & ys, const Episode & episode,
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
	for (const auto & diagnosis : episode.secondary_diagnoses()) {
	    write_yaml_stream(ys, diagnosis, lookup);
	}
	ys << YAML::EndSeq;
    }

    if (episode.secondary_procedures().size() > 0) {
	ys << YAML::Key << "secondary_procedures"
	   << YAML::Value
	   << YAML::BeginSeq;
	for (const auto & diagnosis : episode.secondary_procedures()) {
	    write_yaml_stream(ys, diagnosis, lookup);
	}
	ys << YAML::EndSeq;
    }
    ys << YAML::EndMap;
}

/// Write a spell to a yaml stream.
void write_yaml_stream(YAML::Emitter & ys, const Spell & spell,
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
	for (const auto & episode : spell.episodes()) {
	    write_yaml_stream(ys, episode, lookup);
	}
	ys << YAML::EndSeq;
    }
    ys << YAML::EndMap;
}

void print_sql_query(const std::string & config_path) {
    //std::string config_path_str{Rcpp::as<std::string>(config_path)};
    try {
	    auto config{load_config_file(config_path)};
	    auto sql_query{make_acs_sql_query(config["sql_query"], true, std::nullopt)};
	    std::cout << sql_query << std::endl;
    } catch (const std::runtime_error & e) {
	    std::cout << "Failed with error: " << e.what() << std::endl;
    }
}

int add(int i, int j) {
    return i + j;
}

PYBIND11_MODULE(pbtest, m) {
    m.doc() = "pybind11 bindings for acs dataset preprocessor";

    m.def("add", &add, "A function that adds two numbers");
    m.def("print_sql_query", &print_sql_query, "Print the SQL query that will be used to get the underlying data");
}

