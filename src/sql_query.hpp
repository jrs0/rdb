#ifndef SQL_QUERY
#define SQL_QUER

#include "yaml.hpp"

std::string make_acs_sql_query(const YAML::Node & config, bool with_mortality = true) {

    std::stringstream query;

    query << "select ";

    if (config["result_limit"]) {
	std::size_t result_limit{config["result_limit"].as<std::size_t>()};
	std::cout << "Using result limit " << result_limit << std::endl;
	query << "top " << result_limit << " ";
    }
    
    query << "episodes.* ";
    if (with_mortality) {
	query << ", mort.REG_DATE_OF_DEATH as date_of_death,"
	      << ", mort.S_UNDERLYING_COD_ICD10 as cause_of_death,"
	      << ", mort.Dec_Age_At_Death as age_at_death ";
    }
    query << "from (select " 
	  << "AIMTC_Pseudo_NHS as nhs_number,"
	  << "AIMTC_Age as age_at_episode,"
	  << "PBRspellID as spell_id,"
	  << "StartDate_ConsultantEpisode as episode_start,"
	  << "EndDate_ConsultantEpisode as episode_end,"
	  << "AIMTC_ProviderSpell_Start_Date as spell_start,"
	  << "AIMTC_ProviderSpell_End_Date as spell_end ";

    // Add all the diagnosis columns
    auto diagnoses{expect_string_vector(config["parser_config"]["diagnoses"], "source_columns")};
    for (const auto & diagnosis : diagnoses) {
	query <<  "," << diagnosis;
    }
    auto procedures{expect_string_vector(config["parser_config"]["procedures"], "source_columns")};
    for (const auto & procedure : procedures) {
	query << "," << procedure;
    }

    query << " from abi.dbo.vw_apc_sem_001 "
	  << "where datalength(AIMTC_Pseudo_NHS) > 0 "
	  << "and datalength(pbrspellid) > 0 "
	  << ") as episodes ";
    if (with_mortality) {
	query << "left join abi.civil_registration.mortality as mort "
	      << "on episodes.nhs_number = mort.derived_pseudo_nhs ";
    }
    query << "order by nhs_number, spell_id; ";
    
    return query.str();
}



#endif
