#ifndef SQL_QUERY
#define SQL_QUERY

#include "yaml.hpp"
#include <optional>

/**
 * \brief Make the SQL query for the ACS dataset
 *
 * The config file is the "sql_query" block. It should contains primary_diagnosis
 * and primary_procedure keys, and secondary_diagnoses and secondary_procedures
 * lists. These are all column names, that will be mapped to the names used
 * by the Episode constructor
 */
std::string make_acs_sql_query(const YAML::Node & config, bool with_mortality,
			       const std::optional<std::string> & nhs_number) {

    std::stringstream query;

    query << "select ";

    if (config["result_limit"]) {
	std::size_t result_limit{config["result_limit"].as<std::size_t>()};
	query << "top " << result_limit << " ";
    }
    
    query << "episodes.* ";
    if (with_mortality) {
	query << ", mort.REG_DATE_OF_DEATH as date_of_death"
	      << ", mort.S_UNDERLYING_COD_ICD10 as cause_of_death"
	      << ", mort.Dec_Age_At_Death as age_at_death ";
    }
    query << std::endl
	  << "from (select " 
	  << "AIMTC_Pseudo_NHS as nhs_number,"
	  << "AIMTC_Age as age_at_episode,"
	  << "PBRspellID as spell_id,"
	  << "StartDate_ConsultantEpisode as episode_start,"
	  << "EndDate_ConsultantEpisode as episode_end,"
	  << "AIMTC_ProviderSpell_Start_Date as spell_start,"
	  << "AIMTC_ProviderSpell_End_Date as spell_end,"
	  << std::endl;

    // Add the primary diagnosis and procedure columns
    query << config["primary_diagnosis"] << " as primary_diagnosis," << std::endl;
    query << config["primary_procedure"] << " as primary_procedure " << std::endl;

    // Get all the secondary diagnosis columns
    std::size_t count{0};
    for (const auto & diagnosis : config["secondary_diagnoses"]) {
	query <<  "," << diagnosis << " as secondary_diagnosis_" << count++ << std::endl;
    }
    count = 0;
    for (const auto & procedure : config["secondary_procedures"]) {
	query << "," << procedure << " as secondary_procedure_" << count++
	      << std::endl;
    }

    query << " from abi.dbo.vw_apc_sem_001 "
	  << "where datalength(AIMTC_Pseudo_NHS) > 0 "
	  << "and datalength(pbrspellid) > 0 "
	  << ") as episodes ";
    if (with_mortality) {
	query << "left join abi.civil_registration.mortality as mort "
	      << "on episodes.nhs_number = mort.derived_pseudo_nhs ";
    }

    if (nhs_number.has_value()) {
	query << "where nhs_number = '" << *nhs_number << "' ";
    }

    query << "order by nhs_number, spell_id ";

    return query.str();
}



#endif
