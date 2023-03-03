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

// To be moved out of here
#include <Rcpp.h>

#define DEBUG

void debug_msg(const std::string & msg) {
#ifdef DEBUG
    std::cout << msg << std::endl;
#endif
}

void handle_diagnostic_record(SQLHANDLE handle, SQLSMALLINT type, RETCODE ret_code)
{
    SQLSMALLINT iRec = 0;
    SQLINTEGER  iError;
    WCHAR wszMessage[1000];
    WCHAR wszState[SQL_SQLSTATE_SIZE+1];

    if (ret_code == SQL_INVALID_HANDLE) {
	throw std::runtime_error("Invalid handle");
    }

    std::stringstream ss;
    while (SQLGetDiagRec(type, handle, ++iRec, (SQLCHAR*)wszState, &iError,
			 (SQLCHAR*)wszMessage,
                         (SQLSMALLINT)(sizeof(wszMessage) / sizeof(WCHAR)),
                         (SQLSMALLINT *)NULL) == SQL_SUCCESS) {
	// Hide data truncated..
	if (wcsncmp(wszState, L"01004", 5)) {
	    ss << "state: " << wszState
	       << " message: " << wszMessage
	       << " (" << iError;
	    // TODO: format like this: 
	    //L"[%5.5s] %s (%d)\n", wszState, wszMessage, iError);
	}
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
    case SQL_ERROR:
	// Throws runtime_error
	handle_diagnostic_record(handle, type, ret_code);
	
    default:
	throw std::runtime_error("Unexpected return code in SQLRETURN");
    }
}

/// A simple SQL
class SQLConnection {

public:

    /// Create an SQL connection to a data source
    SQLConnection(const std::string & dsn) : dsn_{dsn} {

	SQLRETURN r;

	// Allocate global environment handle
	r = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv_);
	try {
	    result_ok(henv_, SQL_HANDLE_ENV, r);
	    debug_msg("Allocated global env");
	} catch (const std::runtime_error & e) {
	    free_all();
	    throw std::runtime_error("Failed to alloc environment: " + e.what())
	}
	    

	// Set environment attributes
	r = SQLSetEnvAttr(henv_, SQL_ATTR_ODBC_VERSION,
			  (SQLPOINTER)SQL_OV_ODBC3, 0);
	try {
	    result_ok(henv_, SQL_HANDLE_ENV, r);
	    debug_msg("Set environment variables");
	} catch (const std::runtime_error & e) {
	    free_all();
	    throw std::runtime_error("Failed to set env variable ODBC3: " + e.what())
	}
	
	// Allocate a connection
	r = SQLAllocHandle(SQL_HANDLE_DBC, henv_, &hdbc_);
	try {
	    result_ok(henv_, SQL_HANDLE_ENV, r);
	    debug_msg("Allocated the connection handle");
	} catch (const std::runtime_error & e) {
	    free_all();
	    throw std::runtime_error("Failed to alloc connection handle: " + e.what())
	}
	
	// Make the connection
	r = SQLConnect(hdbc_, (SQLCHAR*)dsn_.c_str(), SQL_NTS, NULL, 0, NULL, 0);
	try {
	    result_ok(henv_, SQL_HANDLE_DBC, r);
	    debug_msg("Established connection");
	} catch (const std::runtime_error & e) {
	    free_all();
	    throw std::runtime_error("Failed to connect to DSN: " + e.what())
	}

	// Allocate statement handle
	r = SQLAllocHandle(SQL_HANDLE_STMT, hdbc_, &hstmt_);
	try {
	    result_ok(hdbc_, SQL_HANDLE_DBC, r);
	    debug_msg("Allocated statement handle");
	} catch (const std::runtime_error & e) {
	    free_all();
	    throw std::runtime_error("Failed to allocate statement handle: " + e.what())
	}
    }

    ~SQLConnection() {
	debug_msg("Freeing all handles");
	free_all();
    }

    /// Submit an SQL query
    std::size_t query(const std::string & query) {

	// SQL_NTS for null-terminated string (query)
	if(!result_ok(SQLExecDirect(hstmt_, (SQLCHAR*)query.c_str(), SQL_NTS))) {
	    throw std::runtime_error("Failed to add query for direct execution");
	}
	debug_msg("Added statement for execution");

	// Obtain the results
	SQLSMALLINT num_results = 0;
	if (!result_ok(SQLNumResultCols(hstmt_, &num_results))) {
	    throw std::runtime_error("Failed to get the number of returned rows");
	}

	return num_results;
    }
    
private:
    std::string dsn_; ///< Data source name
    SQLHENV henv_; ///< Global environment handle
    SQLHDBC hdbc_; ///< Connection handle
    SQLHSTMT hstmt_; ///< Statement handle

    
    /// Free any resources that have been allocated. For
    /// failed construction and destruction
    void free_all() {
	if (hstmt_) {
	    SQLFreeHandle(SQL_HANDLE_STMT, hstmt_);
	}
    
	if (hdbc_) {
	    SQLDisconnect(hdbc_);
	    SQLFreeHandle(SQL_HANDLE_DBC, hdbc_);
	}
	
	if (henv_) {
	    SQLFreeHandle(SQL_HANDLE_ENV, henv_);
	}
    } 
};

// [[Rcpp::export]]
void try_connect(const Rcpp::CharacterVector & query_string) {
    try {
	std::string query = Rcpp::as<std::string>(query_string); 
	
	// Make the connection
	SQLConnection con("xsw");

	// Attempt to add a statement for direct execution
	std::size_t num_results = con.query(query);
	Rcpp::Rcout << "Got " << num_results << " results from query" << std::endl;
	
    } catch (const std::runtime_error & e) {
	Rcpp::Rcout << "Failed with error: " << e.what() << std::endl;
    }
}
