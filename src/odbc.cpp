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

#include <vector>
#include <map>

#include "row_buffer.hpp"

// To be moved out of here
#include <Rcpp.h>

/// A simple SQL
class SQLConnection {

public:

    /// Create an SQL connection to a data source
    SQLConnection(const std::string & dsn)
	: dsn_{dsn}, env_{std::make_shared<EnvHandle>()} {
	env_->set_attribute((SQLPOINTER)SQL_OV_ODBC3, 0);
	dbc_ = std::make_shared<ConHandle>(env_, dsn);
	stmt_ = std::make_shared<StmtHandle>(dbc_);
    }

    /// Submit an SQL query and get the result back as a set of
    /// columns (with column names).
    RowBuffer execute_direct(const std::string & query) {
	stmt_->exec_direct(query);
	return RowBuffer{stmt_};
    }
    
private:
    std::string dsn_; ///< Data source name
    std::shared_ptr<EnvHandle> env_; ///< Global environment handle
    std::shared_ptr<ConHandle> dbc_; ///< Connection handle
    std::shared_ptr<StmtHandle> stmt_; ///< Statement handle

};

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

	YAML::Node top_level_category_yaml = YAML::LoadFile("icd10_example.yaml");
	TopLevelCategory top_level_category{top_level_category_yaml};

	while(true) {
	    try {

		// Try to fetch the next row (throws if none left)
		auto row{row_buffer.try_next_row()};

		// NOW row HAS VALUES, DO PARSING HERE
		row[0] = top_level_category.get_code_name(row[0]);
		    
		// Copy into the table
		for (std::size_t col{0}; col < row_buffer.size(); col++) {
		    auto col_name{column_names[col]};
		    auto col_value{row[col]};
		    table[col_name].push_back(col_value); 
		}
			
	    } catch (const std::logic_error & e) {
		std::cout << e.what() << std::endl;
		break;
	    }
	}
        
	// Convert the table to R format
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
void parse_icd(const Rcpp::CharacterVector & icd10_file_character,
	       const Rcpp::CharacterVector & code_character) {

    std::string icd10_file = Rcpp::as<std::string>(icd10_file_character);     
    std::string code = Rcpp::as<std::string>(code_character);
    
    try {
	YAML::Node top_level_category_yaml = YAML::LoadFile(icd10_file);
	TopLevelCategory top_level_category{top_level_category_yaml};
	std::cout << top_level_category.get_code_name(code) << std::endl;;
    } catch(const YAML::BadFile& e) {
	throw std::runtime_error("Bad YAML file");
    } catch(const YAML::ParserException& e) {
	throw std::runtime_error("YAML parsing error");
    } catch(const std::runtime_error & e) {
	Rcpp::Rcout << "Failed with error: " << e.what() << std::endl;
    }
}
