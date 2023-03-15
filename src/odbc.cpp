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

#include "stmt_handle.hpp"
#include "yaml.hpp"
#include "category.hpp"

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
    std::map<std::string, std::vector<std::string>> query(const std::string & query) {

	stmt_->exec_direct(query);

	std::size_t num_columns{stmt_->num_columns()};
	
	// Loop over the columns (note: indexed from 1!)
	// Get the column types
	std::vector<ColBinding> col_bindings;

	// For now, the table is a map from column names to vectors
	// of strings (can handle types later)
	std::map<std::string, std::vector<std::string>> table;

	// Get all the column names. This is where you might do
	// column name remapping.
	for (std::size_t n = 1; n <= num_columns; n++) {
	    std::string colname{stmt_->column_name(n)};
	    col_bindings.push_back(stmt_->make_binding(n));
	}

	// Fetch all the rows. This is where "local" preprocessing of
	// values would go (i.e. parsing of ICD values). 
	std::size_t num_rows{0};
	while(true) {

	    /// Attempt to fetch a row and break if no rows left
	    if(not stmt_->fetch()) {
		break;
	    }
	    
	    // Print the row of data from the column bindings
	    for (auto & bind : col_bindings) {
		std::string value;
		try {
		    value = bind.read_buffer();
		} catch (const std::logic_error &) {
		    value = "NULL";
		}
		table[bind.col_name()].push_back(value);
	    }
	    num_rows++;
	}
	return table;
    }
    
private:
    std::string dsn_; ///< Data source name
    std::shared_ptr<EnvHandle> env_; ///< Global environment handle
    std::shared_ptr<ConHandle> dbc_; ///< Connection handle
    std::shared_ptr<StmtHandle> stmt_; ///< Statement handle

};

// [[Rcpp::export]]
void parse_icd(const Rcpp::CharacterVector & icd10_file_character,
	       const Rcpp::CharacterVector & code_character) {

    std::string icd10_file = Rcpp::as<std::string>(icd10_file_character);     
    std::string code = Rcpp::as<std::string>(code_character);
    
    try {
	YAML::Node top_level_category_yaml = YAML::LoadFile(icd10_file);
	TopLevelCategory top_level_category{top_level_category_yaml};
	std::cout << top_level_category.parse_code(code) << std::endl;;
    } catch(const YAML::BadFile& e) {
	throw std::runtime_error("Bad YAML file");
    } catch(const YAML::ParserException& e) {
	throw std::runtime_error("YAML parsing error");
    } catch(const std::runtime_error & e) {
	Rcpp::Rcout << "Failed with error: " << e.what() << std::endl;
    }
}

// [[Rcpp::export]]
Rcpp::List try_connect(const Rcpp::CharacterVector & dsn_character,
		       const Rcpp::CharacterVector & query_character) {
    try {
	std::string dsn = Rcpp::as<std::string>(dsn_character); 
	std::string query = Rcpp::as<std::string>(query_character); 

	// Make the connection
	SQLConnection con(dsn);

	// Attempt to add a statement for direct execution
	auto table{con.query(query)};

	Rcpp::List table_list;;
	// Loop over columns

	for (const auto & [col_name, col_values] : table) {
	    
	    Rcpp::CharacterVector col_values_vector(col_values.begin(), col_values.end());
	    table_list[col_name] = col_values_vector;
	}

	return table_list;
	
    } catch (const std::runtime_error & e) {
	Rcpp::Rcout << "Failed with error: " << e.what() << std::endl;
	return Rcpp::List{};
    }
}
