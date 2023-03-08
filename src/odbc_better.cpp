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

#include <string>
#include <sstream>
#include <iostream>
#include <stdexcept>

#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <memory>

// To be moved out of here
#include <Rcpp.h>

#define DEBUG

void debug_msg(const std::string & msg) {
#ifdef DEBUG
    std::cout << msg << std::endl;
#endif
}

// Not currently return the correct messages, but throws runtime_error if
// there is a problem
void handle_diagnostic_record(SQLHANDLE handle, SQLSMALLINT type, RETCODE ret_code)
{

    if (ret_code == SQL_INVALID_HANDLE) {
	throw std::runtime_error("Invalid handle");
    }

    SQLINTEGER error;
    //std::string sql_state(SQL_SQLSTATE_SIZE + 1, '.');
    //std::string sql_message(1000, '.');
    SQLCHAR sql_state[SQL_SQLSTATE_SIZE + 1];
    SQLCHAR sql_message[1000];
    SQLSMALLINT rec = 0;
    SQLSMALLINT message_size;
    std::stringstream ss;
    while (SQLGetDiagRec(type, handle, ++rec, sql_state, &error,
			 sql_message, 1000,
                         &message_size) == SQL_SUCCESS) {
	// Hide data truncated..
	//std::cout << message_size << std::endl;
	//sql_message.resize(message_size);
	ss << "state: " << sql_state
	   << " message: " << sql_message
	   << " (" << error << ")"
	   << std::endl;
	// TODO: format like this: 
	//	    //L"[%5.5s] %s (%d)\n", wszState, wszMessage, iError);
	//}
    }
    
    throw std::runtime_error(ss.str());
}

/// Test an ODBC return code, return
/// true for success and false for failure
bool result_ok(SQLHANDLE handle, SQLSMALLINT type, SQLRETURN ret_code) {
    switch(ret_code) {
    case SQL_SUCCESS_WITH_INFO:
	return true;
    case SQL_SUCCESS:
	return true;
    case SQL_INVALID_HANDLE:
	throw std::runtime_error("SQLRETURN Invalid Handle");
    case SQL_ERROR:
	// Throws runtime_error
	handle_diagnostic_record(handle, type, ret_code);
	
    default:
	std::stringstream ss;
	ss << "Unexpected return code in SQLRETURN: " << ret_code; 
	throw std::runtime_error(ss.str());
    }
}

/// Global environment handle
class EnvHandle {
public:
    EnvHandle() {
	SQLRETURN r = SQLAllocHandle(SQL_HANDLE_ENV,
				     SQL_NULL_HANDLE,
				     &henv_);
	try {
	    result_ok(henv_, SQL_HANDLE_ENV, r);
	    debug_msg("Allocated global env");
	} catch (const std::runtime_error & e) {
	    std::stringstream ss;
	    ss << "Failed to alloc environment: " << e.what();
	    throw std::runtime_error(ss.str());
	}
    }
    void set_attribute(SQLPOINTER value_ptr, SQLINTEGER str_len) {
	SQLRETURN r = SQLSetEnvAttr(henv_, SQL_ATTR_ODBC_VERSION,
				    value_ptr, str_len);
	try {
	    result_ok(henv_, SQL_HANDLE_ENV, r);
	    debug_msg("Set environment variables");
	} catch (const std::runtime_error & e) {
	    std::stringstream ss;
	    ss << "Failed to set env variable: " << e.what();
	    throw std::runtime_error(ss.str());
	}	
    }
    SQLHENV get_handle() {
	return henv_;
    }
    ~EnvHandle() {
	debug_msg("Freeing environment handle");
    	if (henv_) {
	    SQLFreeHandle(SQL_HANDLE_ENV, henv_);
	}	
    }
    EnvHandle(const EnvHandle&) = delete;
    EnvHandle& operator=(const EnvHandle&) = delete;
private:
    SQLHENV henv_{NULL};
};

class ConHandle {
public:
    ConHandle(std::shared_ptr<EnvHandle> henv, const std::string & dsn)
	: henv_{henv}
    {
	SQLRETURN r = SQLAllocHandle(SQL_HANDLE_DBC, henv_->get_handle(), &hdbc_);
	try {
	    result_ok(henv_->get_handle(), SQL_HANDLE_ENV, r);
	    debug_msg("Allocated the connection handle");
	} catch (const std::runtime_error & e) {
	    std::stringstream ss;
	    ss << "Failed to alloc connection handle: " << e.what();
	    throw std::runtime_error(ss.str());
	}

	// Make the connection
	r = SQLConnect(hdbc_, (SQLCHAR*)dsn.c_str(), SQL_NTS, NULL, 0, NULL, 0);
	try {
	    result_ok(hdbc_, SQL_HANDLE_DBC, r);
	    debug_msg("Established connection");
	} catch (const std::runtime_error & e) {
	    free_handle();
	    std::stringstream ss;
	    ss << "Failed to connect to DSN: " << e.what();
	    throw std::runtime_error(ss.str());
	}
    }
    SQLHDBC get_handle() {
	return hdbc_;
    }
    ~ConHandle() {
	free_handle();
    }
    ConHandle(const ConHandle&) = delete;
    ConHandle& operator=(const ConHandle&) = delete;
private:
    void free_handle() {
	debug_msg("Freeing connection handle");
	if (hdbc_) {
	    SQLDisconnect(hdbc_);
	    SQLFreeHandle(SQL_HANDLE_DBC, hdbc_);
	}	
    }
    
    std::shared_ptr<EnvHandle> henv_; /// Keep alive for ConHandle
    SQLHDBC hdbc_{NULL};
};

class StmtHandle {
public:
    StmtHandle(std::shared_ptr<ConHandle> hdbc)
	: hdbc_{hdbc}
    {
	SQLRETURN r = SQLAllocHandle(SQL_HANDLE_STMT, hdbc_->get_handle(), &hstmt_);
	try {
	    result_ok(hdbc_->get_handle(), SQL_HANDLE_DBC, r);
	    debug_msg("Allocated statement handle");
	} catch (const std::runtime_error & e) {
	    std::stringstream ss;
	    ss << "Failed to alloc statement handle: " << e.what();
	    throw std::runtime_error(ss.str());
	}

    }
    SQLHSTMT get_handle() {
	return hstmt_;
    }
    void exec_direct(const std::string & query) {
    	// SQL_NTS for null-terminated string (query)
	SQLRETURN r = SQLExecDirect(hstmt_, (SQLCHAR*)query.c_str(), SQL_NTS);
	try {
	    result_ok(hstmt_, SQL_HANDLE_STMT, r);
	} catch (std::runtime_error & e) {
	    std::stringstream ss;
	    ss << "Failed to add query for direct execution: " << e.what();
	    throw std::runtime_error(ss.str());
	}
	debug_msg("Added statement for execution");
    }
    /// Get the number of returned columns (sql_direct comes first)
    std::size_t num_columns() {
	// Obtain the results
	SQLSMALLINT num_columns = 0;
	SQLRETURN r = SQLNumResultCols(hstmt_, &num_columns);
	try {
	    result_ok(hstmt_, SQL_HANDLE_STMT, r);
	} catch (std::runtime_error & e) {
	    std::stringstream ss;
	    ss << "Failed to get number of returned columns: " << e.what();
	    throw std::runtime_error(ss.str());
	}
	return num_columns;
    }

    /// Remember columns are indexed from 1
    std::string column_name(std::size_t index) {
	if (index == 0) {
	    throw std::runtime_error("Column index 0 out of range");
	}
	SQLSMALLINT column_name_length = 0;
	SQLRETURN r = SQLColAttribute(hstmt_, index, SQL_DESC_NAME, NULL, 0,
			    &column_name_length, NULL);
	try {
	    result_ok(hstmt_, SQL_HANDLE_STMT, r);
	} catch (std::runtime_error & e) {
	    std::stringstream ss;
	    ss << "Failed to get the length of column "
	       << index << ": " << e.what();
	    throw std::runtime_error(ss.str());
	}

	// Get the column name itself
#define COLUMN_NAME_BUF_LEN 100
	SQLCHAR column_name_buf[COLUMN_NAME_BUF_LEN];
	r = SQLColAttribute(hstmt_, index, SQL_DESC_NAME,
			    column_name_buf, COLUMN_NAME_BUF_LEN,
			    NULL, NULL);
	std::string column_name((char*)column_name_buf);
	try {
	    result_ok(hstmt_, SQL_HANDLE_STMT, r);
	} catch (std::runtime_error & e) {
	    std::stringstream ss;
	    ss << "Failed to get column name for col "
	       << index << ": " << e.what();
	    throw std::runtime_error(ss.str());
	}
	return std::string((char*)column_name_buf);
    }
    
    ~StmtHandle() {
	debug_msg("Freeing statement handle");
	if (hstmt_) {
	    SQLFreeHandle(SQL_HANDLE_STMT, hstmt_);
	}
    }
    StmtHandle(const StmtHandle&) = delete;
    StmtHandle& operator=(const StmtHandle&) = delete;
private:
    std::shared_ptr<ConHandle> hdbc_; /// Keep alive for this
    SQLHSTMT hstmt_; ///< Statement handle
};


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

    /// Submit an SQL query
    std::size_t query(const std::string & query) {

	stmt_->exec_direct(query);

	std::size_t num_columns{stmt_->num_columns()};
	
	// Loop over the columns (note: indexed from 1!)
	// Get the column types
	for (std::size_t n = 1; n <= num_columns; n++) {
	    std::cout << "Got column name: " << stmt_->column_name(n) << std::endl;
	}
	return num_columns;
    }
    
private:
    std::string dsn_; ///< Data source name
    std::shared_ptr<EnvHandle> env_; ///< Global environment handle
    std::shared_ptr<ConHandle> dbc_; ///< Connection handle
    std::shared_ptr<StmtHandle> stmt_; ///< Statement handle

};

// [[Rcpp::export]]
void try_connect(const Rcpp::CharacterVector & dsn_character,
		 const Rcpp::CharacterVector & query_character) {
    try {
	std::string dsn = Rcpp::as<std::string>(dsn_character); 
	std::string query = Rcpp::as<std::string>(query_character); 

	// Make the connection
	SQLConnection con(dsn);

	// Attempt to add a statement for direct execution
	std::size_t num_columns = con.query(query);
	Rcpp::Rcout << "Got " << num_columns
		    << " columns from the query" << std::endl;
	
    } catch (const std::runtime_error & e) {
	Rcpp::Rcout << "Failed with error: " << e.what() << std::endl;
    }
}
