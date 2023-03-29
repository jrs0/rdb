#include <iostream>
#include "acs.hpp"

int main() {
    try {
	std::string dsn{"xsw"}; 
	std::string query{"select top 10 diagnosisprimary_icd,aimtc_pseudo_nhs,"
	    "pbrspellid,enddate_consultantepisode from abi.dbo.vw_apc_sem_001"}; 

	// Make the connection
	SQLConnection con(dsn);

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
