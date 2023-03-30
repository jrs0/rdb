#ifndef EPISODE_ROW
#define EPISODE_ROW

#include <map>
#include <vector>
#include "sql_types.hpp"
#include <random>
#include "clinical_code.hpp"

/// A mock row for testing the Episode row constructor
class EpisodeRowBuffer {
public:
    EpisodeRowBuffer() {
	columns_["episode_start"] = Timestamp{0};
	columns_["episode_end"] = Timestamp{0};
    }

    EpisodeRowBuffer(const Timestamp & start, const Timestamp & end) {
	columns_["episode_start"] = start;
	columns_["episode_end"] = end;	
    }

    std::size_t size() const {
	return columns_.size();
    }

    void set_random_fields(const ClinicalCodeParser & parser,
			   std::uniform_random_bit_generator auto & gen) {
	set_primary_procedure(parser.random_procedure(gen));
	set_primary_diagnosis(parser.random_diagnosis(gen));
    }

    void set_primary_diagnosis(const std::string & raw) {
        columns_["primary_diagnosis"] = Varchar{raw};	
    }

    void set_primary_procedure(const std::string & raw) {
        columns_["primary_procedure"] = Varchar{raw};	
    }

    void set_secondary_procedures(const std::vector<std::string> & vec) {
	for (const auto & raw : vec) {
	    push_secondary_procedure(raw);
	}
    }

    void set_secondary_diagnoses(const std::vector<std::string> & vec) {
	for (const auto & raw : vec) {
	    push_secondary_diagnosis(raw);
	}
    }
    
    void push_secondary_procedure(const std::string & raw) {
	auto column_name{"secondary_procedure_"
			 + std::to_string(num_secondary_procedures_++)};
	columns_[column_name] = Varchar{raw};
    }

    void push_secondary_diagnosis(const std::string & raw) {
	auto column_name{"secondary_diagnosis_"
			 + std::to_string(num_secondary_diagnoses_++)};
	columns_[column_name] = Varchar{raw};
    }
    
    
    /// Throws out_of_range if column does not exist, and
    /// bad_variant_access if T is not this column's type
    template<typename T>
    T at(std::string column_name) const {
        return std::get<T>(columns_.at(column_name));
    }
    
private:
    std::map<std::string, SqlType> columns_;
    std::size_t num_secondary_procedures_{0};
    std::size_t num_secondary_diagnoses_{0};
};


#endif
