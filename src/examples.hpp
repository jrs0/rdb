#ifndef EXAMPLES_HPP
#define EXAMPLES_HPP

#include "acs.hpp"

void test_query() {
    try {
	std::string query{"select top 10 diagnosisprimary_icd,aimtc_pseudo_nhs,"
	    "pbrspellid,enddate_consultantepisode from abi.dbo.vw_apc_sem_001"}; 

	// Make the connection
        auto cred{YAML::LoadFile("/root/db.secret.yaml")};
        SQLConnection con(cred);

	// Make the row buffer
	auto row{con.execute_direct(query)};

	int c{0};
	while (true) {
	    try {
		// Fetch a row
		row.fetch_next_row();
		
		// Get results
		column<Integer>("aimtc_pseudo_nhs", row).print();
		column<Varchar>("pbrspellid", row).print();
		column<Varchar>("diagnosisprimary_icd", row).print();
		std::cout << c++ << " ";
		column<Timestamp>("enddate_consultantepisode", row).print();
		
	    } catch (const std::logic_error & e) {
		std::cout << e.what() << std::endl;
		break;
	    }
	}
    } catch (const std::runtime_error & e) {
	std::cout << "Failed with error: " << e.what() << std::endl;
    } catch (const NullValue & ) {
	std::cout << "An unhandled NULL value occured"
		  << std::endl;	
    }
}

void fetch_acs() {
    auto config_path{"config.yaml"};
    try {
	YAML::Node config = YAML::LoadFile(config_path);

	std::cout << "Configuration: " << std::endl
		  << config << std::endl << std::endl;

	// Get the records, one per index event, along with
	// procedures/diagnoses before and after
	auto records{get_acs_records(config)};
	
    } catch(const YAML::BadFile& e) {
	throw std::runtime_error("Bad YAML file");
    } catch(const YAML::ParserException& e) {
	throw std::runtime_error("YAML parsing error");
    } catch (const std::runtime_error & e) {
	std::cout << "Failed with error "  << e.what() << std::endl;
    } catch (const NullValue & ) {
	std::cout << "An unhandled NULL value occured"
		  << std::endl;
    }
}


#endif
