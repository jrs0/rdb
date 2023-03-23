// To understand the contents of this file, see the Microsoft ODBC
// documentation starting here:
//
// "https://learn.microsoft.com/en-us/sql/relational-databases/
//  native-client-odbc-communication/communicating-with-sql-
//  server-odbc?view=sql-server-ver16"
//
// in conjunction with the example here:
//
// "https://learn.microsoft.com/en-us/sql/connect/odbc/cpp-code
//  -example-app-connect-access-sql-db?view=sql-server-ver16"
//

#include "random.hpp"
#include "mem_row_buffer.hpp" 

// [[Rcpp::export]]
void in_mem_test() {
    auto gen{Generator<std::size_t,0,1>(Seed<>())};

    YAML::Node config = YAML::LoadFile("config.yaml");
    InMemoryRowBuffer row{config};

    
    
    // TopLevelCategory procedures{opcs4};
    // std::cout << "Random OPCS: " << procedures.random_code(gen)
    // 	      << std::endl; 
    // YAML::Node icd10 = YAML::LoadFile("icd10.yaml");
    // TopLevelCategory diagnoses{icd10};
    // for (std::size_t n{0}; n < 10; n++) {
    // 	std::cout << "Random ICD: " << diagnoses.random_code(gen)
    // 		  << std::endl;
    // }
}


/*

#include "acs.hpp"
#include "random.hpp"

// [[Rcpp::export]]
void make_acs_dataset(const Rcpp::CharacterVector & config_path_chr) {

    auto config_path{Rcpp::as<std::string>(config_path_chr)};
    try {
	YAML::Node config = YAML::LoadFile(config_path);
	Acs acs{config};
    } catch(const YAML::BadFile& e) {
	throw std::runtime_error("Bad YAML file");
    } catch(const YAML::ParserException& e) {
	throw std::runtime_error("YAML parsing error");
    } catch (const std::runtime_error & e) {
	Rcpp::Rcout << "Failed with error: " << e.what() << std::endl;
    }
}

// [[Rcpp::export]]
void test_random_code() {
    auto gen{Generator<std::size_t,0,1>(Seed<>())};

    YAML::Node config = YAML::LoadFile("config.yaml");
    InMemoryRowBuffer row{config};
    
    // TopLevelCategory procedures{opcs4};
    // std::cout << "Random OPCS: " << procedures.random_code(gen)
    // 	      << std::endl; 
    // YAML::Node icd10 = YAML::LoadFile("icd10.yaml");
    // TopLevelCategory diagnoses{icd10};
    // std::cout << "Random ICD: " << diagnoses.random_code(gen)
    // 	      << std::endl;
}



// [[Rcpp::export]]
Rcpp::List try_connect(const Rcpp::CharacterVector & dsn_character,
		       const Rcpp::CharacterVector & query_character) {
    try {
	std::string dsn = Rcpp::as<std::string>(dsn_character); 
	std::string query = Rcpp::as<std::string>(query_character); 

	// Make the connection
	SQLConnection con(dsn);

	// Fetch the row buffer (column names + allocated buffer space for one row)
	auto row_buffer{con.execute_direct(query)};
	auto column_names{row_buffer.column_names()};
	
	// Make a (column-major) table to store the fetched rows
	std::map<std::string, std::vector<std::string>> table;

	YAML::Node top_level_category_yaml = YAML::LoadFile("opcs4.yaml");
	TopLevelCategory top_level_category{top_level_category_yaml};

	while(true) {
	    try {

		// Try to fetch the next row (throws if none left)
		row_buffer.fetch_next_row();

		try {
		    // NOW row HAS VALUES, DO PARSING HERE
		    // Parsing 100,000 rows takes 54 seconds, vs 2s without
		    // parsing. 50,000 takes 28 seconds, so it scales approximately
		    // linearly.
		    //row[1] = top_level_category.get_code_prop(row[0], false);
		} catch (const std::runtime_error & e) {
		    //row[1] = row[0] + std::string{" [INVALID]"};
		}
		
		// Copy into the table
		for (std::size_t col{0}; col < row_buffer.size(); col++) {
		    auto col_name{column_names[col]};
		    auto col_value{row_buffer.at(col_name)};
		    table[col_name].push_back(col_value); 
		}
			
	    } catch (const std::logic_error & e) {
		std::cout << e.what() << std::endl;
		break;
	    }
	}

	std::cout << "Finished fetch: cache size = " << top_level_category.cache_size() << std::endl;
	
	// Convert the table to R format. This is not the slow part.
	Rcpp::List table_list;
	for (const auto & [col_name, col_values] : table) {
	    
	    Rcpp::CharacterVector col_values_vector(col_values.begin(), col_values.end());
	    table_list[col_name] = col_values_vector;
	}

	return table_list;

    } catch(const YAML::BadFile& e) {
	throw std::runtime_error("Bad YAML file");
    } catch(const YAML::ParserException& e) {
	throw std::runtime_error("YAML parsing error");
    } catch (const std::runtime_error & e) {
	Rcpp::Rcout << "Failed with error: " << e.what() << std::endl;
	return Rcpp::List{};
    }
}

// [[Rcpp::export]]
void parse_code(const Rcpp::CharacterVector & icd10_file_character,
		const Rcpp::CharacterVector & code_character) {

    std::string icd10_file = Rcpp::as<std::string>(icd10_file_character);     
    std::string code = Rcpp::as<std::string>(code_character);
    
    try {
	YAML::Node top_level_category_yaml = YAML::LoadFile(icd10_file);
	TopLevelCategory top_level_category{top_level_category_yaml};
	std::cout << top_level_category.get_code_prop(code, true) << std::endl;;
    } catch(const YAML::BadFile& e) {
	throw std::runtime_error("Bad YAML file");
    } catch(const YAML::ParserException& e) {
	throw std::runtime_error("YAML parsing error");
    } catch(const std::runtime_error & e) {
	Rcpp::Rcout << "Failed with error: " << e.what() << std::endl;
    }
}

*/
