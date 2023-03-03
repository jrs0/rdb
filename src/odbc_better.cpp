#include <string>
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

/// Test an ODBC return code, return
/// true for success and false for failure
bool result_ok(SQLRETURN ret_code) {
    switch(ret_code) {
    case SQL_SUCCESS_WITH_INFO:
	return true;
    case SQL_SUCCESS:
	return true;
    case SQL_ERROR:
	return false;	
    default:
	throw std::runtime_error("Unexpected return code in SQLRETURN");
    }
}

/// A simple SQL
class SQLConnection {

public:
    SQLConnection(const std::string & dsn) : dsn_{dsn} {
	// Allocate global environment handle
	if(!result_ok(SQLAllocHandle(SQL_HANDLE_ENV,
				     SQL_NULL_HANDLE,
				     &henv_))) {
	    free_all();
	    throw std::runtime_error("Failed to alloc ODBC "
				     "global env handle");
	}
	debug_msg("Allocated global env");

	// Set environment attributes
	if(!result_ok(SQLSetEnvAttr(henv_,
				    SQL_ATTR_ODBC_VERSION,
				    (SQLPOINTER)SQL_OV_ODBC3,
				    0))) {
	    free_all();
	    throw std::runtime_error("Failed to set env attributes ODBC3");
	}
	debug_msg("Set environment variables");
	
	// Allocate a connection
	if(!result_ok(SQLAllocHandle(SQL_HANDLE_DBC,
				     henv_,
				     &hdbc_))) {
	    free_all();
	    throw std::runtime_error("Failed to alloc connection handle");
	}	
	debug_msg("Set environment variables");
	
	// Make the connection
	if(!result_ok(SQLConnect(hdbc_,
				 (SQLCHAR*)dsn_.c_str(),
				 dsn_.size(), // Length of server name
				 NULL, 0, NULL, 0))) {
	    free_all();
	    throw std::runtime_error("Failed to connect to DSN");
	}
	debug_msg("Created connection");
	
    }

    ~SQLConnection() {
	debug_msg("Freeing all handles");
	free_all();
    }

private:
    std::string dsn_; ///< Data source name
    SQLHENV henv_; ///< Global environment handle
    SQLHDBC hdbc_; ///< Connection handle

    /// Free any resources that have been allocated. For
    /// failed construction and destruction
    void free_all() {
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
void try_connect() {
    try {
	SQLConnection con("xsw");
    } catch (const std::runtime_error & e) {
	Rcpp::Rcout << "Failed with error: " << e.what() << std::endl;
    }
}
