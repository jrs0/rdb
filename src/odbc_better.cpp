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
    case SQL_ERROR:
	// Throws runtime_error
	handle_diagnostic_record(handle, type, ret_code);
	
    default:
	throw std::runtime_error("Unexpected return code in SQLRETURN");
    }
}

class EnvHandle;
class ConHandle;

class StmtHandle {
public:
    StmtHandle(std::shared_ptr<ConHandle> hdbc)
	: hdbc_{hdbc}
    {
	SQLRETURN r = SQLAllocHandle(SQL_HANDLE_STMT, hdbc_.get(), &hstmt_);
	try {
	    result_ok(hdbc_.get(), SQL_HANDLE_DBC, r);
	    debug_msg("Allocated statement handle");
	} catch (const std::runtime_error & e) {
	    std::stringstream ss;
	    ss << "Failed to alloc statement handle: " << e.what();
	    throw std::runtime_error(ss.str());
	}

    }
    ~StmtHandle() {
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


class ConHandle {
public:
    ConHandle(std::shared_ptr<EnvHandle> henv, const std::string & dsn)
	: henv_{henv}
    {
	SQLRETURN r = SQLAllocHandle(SQL_HANDLE_DBC, henv_.get(), &hdbc_);
	try {
	    result_ok(henv_.get(), SQL_HANDLE_ENV, r);
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
    ~ConHandle() {
	free_handle();
    }
    ConHandle(const ConHandle&) = delete;
    ConHandle& operator=(const ConHandle&) = delete;
private:
    void free_handle() {
	if (hdbc_) {
	    SQLDisconnect(hdbc_);
	    SQLFreeHandle(SQL_HANDLE_DBC, hdbc_);
	}	
    }
    
    std::shared_ptr<EnvHandle> henv_; /// Keep alive for ConHandle
    SQLHENV hdbc_{NULL};
};

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
    ~EnvHandle() {
    	if (henv_) {
	    SQLFreeHandle(SQL_HANDLE_ENV, henv_);
	}	
    }
    EnvHandle(const EnvHandle&) = delete;
    EnvHandle& operator=(const EnvHandle&) = delete;
private:
    SQLHENV henv_{NULL};
};

/// A simple SQL
class SQLConnection {

public:

    /// Create an SQL connection to a data source
    SQLConnection(const std::string & dsn)
	: dsn_{dsn}, henv_{std::make_shared<EnvHandle>()} {

	SQLRETURN r;
	
	// Allocate global environment handle
	// r = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv_);
	// try {
	//     result_ok(henv_, SQL_HANDLE_ENV, r);
	//     debug_msg("Allocated global env");
	// } catch (const std::runtime_error & e) {
	//     free_all();
	//     std::stringstream ss;
	//     ss << "Failed to alloc environment: " << e.what();
	//     throw std::runtime_error(ss.str());
	// }
	
	// Set environment attributes
	// r = SQLSetEnvAttr(henv_, SQL_ATTR_ODBC_VERSION,
	// 		  (SQLPOINTER)SQL_OV_ODBC3, 0);
	// try {
	//     result_ok(henv_, SQL_HANDLE_ENV, r);
	//     debug_msg("Set environment variables");
	// } catch (const std::runtime_error & e) {
	//     free_all();
	//     std::stringstream ss;
	//     ss << "Failed to set env variable ODBC: " << e.what();
	//     throw std::runtime_error(ss.str());
	// }
	henv_->set_attribute((SQLPOINTER)SQL_OV_ODBC3, 0);
	
	// Allocate a connection
	// r = SQLAllocHandle(SQL_HANDLE_DBC, henv_, &hdbc_);
	// try {
	//     result_ok(henv_, SQL_HANDLE_ENV, r);
	//     debug_msg("Allocated the connection handle");
	// } catch (const std::runtime_error & e) {
	//     free_all();
	//     std::stringstream ss;
	//     ss << "Failed to alloc connection handle: " << e.what();
	//     throw std::runtime_error(ss.str());
	// }
	hdbc_ = std::make_shared<ConHandle>(henv_, dsn);
	
	// Make the connection
	// r = SQLConnect(hdbc_, (SQLCHAR*)dsn_.c_str(), SQL_NTS, NULL, 0, NULL, 0);
	// try {
	//     result_ok(hdbc_, SQL_HANDLE_DBC, r);
	//     debug_msg("Established connection");
	// } catch (const std::runtime_error & e) {
	//     free_all();
	//     std::stringstream ss;
	//     ss << "Failed to connect to DSN: " << e.what();
	//     throw std::runtime_error(ss.str());
	// }

	// Allocate statement handle
	// r = SQLAllocHandle(SQL_HANDLE_STMT, hdbc_, &hstmt_);
	// try {
	//     result_ok(hdbc_, SQL_HANDLE_DBC, r);
	//     debug_msg("Allocated statement handle");
	// } catch (const std::runtime_error & e) {
	//     free_all();
	//     std::stringstream ss;
	//     ss << "Failed to alloc statement handle: " << e.what();
	//     throw std::runtime_error(ss.str());
	// }
	hstmt_ = std::make_shared<StmtHandle>(hdbc_);

    }

    // ~SQLConnection() {
    // 	debug_msg("Freeing all handles");
    // 	//free_all();
    // }

    /// Submit an SQL query
    std::size_t query(const std::string & query) {

	/*
	// SQL_NTS for null-terminated string (query)
	r = SQLExecDirect(hstmt_, (SQLCHAR*)query.c_str(), SQL_NTS);
	try {
	    result_ok(hstmt_, SQL_HANDLE_STMT, r);
	} catch (std::runtime_error & e) {
	    std::stringstream ss;
	    ss << "Failed to add query for direct execution: " << e.what();
	    throw std::runtime_error(ss.str());
	}
	debug_msg("Added statement for execution");

	// Obtain the results
	SQLSMALLINT num_columns = 0;
	r = SQLNumResultCols(hstmt_, &num_columns);
	try {
	    result_ok(hstmt_, SQL_HANDLE_STMT, r);
	} catch (std::runtime_error & e) {
	    std::stringstream ss;
	    ss << "Failed to get number of returned columns: " << e.what();
	    throw std::runtime_error(ss.str());
	}

	// Loop over the columns (note: indexed from 1!)
	// Get the column types
	for (int col = 1; col <= num_columns; col++) {
	    
	    // Get the length of the column name
	    SQLSMALLINT column_name_length = 0;
	    r = SQLColAttribute(hstmt_, col, SQL_DESC_NAME, NULL, 0,
				&column_name_length, NULL);
	    try {
		result_ok(hstmt_, SQL_HANDLE_STMT, r);
	    } catch (std::runtime_error & e) {
		std::stringstream ss;
		ss << "Failed to get the length of column name "
		   << col << ": " << e.what();
		throw std::runtime_error(ss.str());
	    }

	    // Get the column name itself
#define COLUMN_NAME_BUF_LEN 100
	    SQLCHAR column_name_buf[COLUMN_NAME_BUF_LEN];
	    r = SQLColAttribute(hstmt_, col, SQL_DESC_NAME,
				column_name_buf, COLUMN_NAME_BUF_LEN,
				NULL, NULL);
	    std::string column_name((char*)column_name_buf);
	    try {
		result_ok(hstmt_, SQL_HANDLE_STMT, r);
	    } catch (std::runtime_error & e) {
		std::stringstream ss;
		ss << "Failed to get column name for col "
		   << col << ": " << e.what();
		throw std::runtime_error(ss.str());
	    }	    
	    
	    std::cout << "Column: " << column_name_length << " " << column_name << std::endl;
	    
	    // r = SQLColAttribute(hStmt,
	    // 			iCol++,
	    // 			SQL_DESC_NAME,
	    // 			wszTitle,
	    // 			sizeof(wszTitle), // Note count of bytes!
	    // 			NULL,
	    // 			NULL)));
}
	*/
	return 0;//num_columns;
    }
    
private:
    std::string dsn_; ///< Data source name
    std::shared_ptr<EnvHandle> henv_; ///< Global environment handle
    std::shared_ptr<ConHandle> hdbc_; ///< Connection handle
    std::shared_ptr<StmtHandle> hstmt_; ///< Statement handle

    /*
    /// Free any resources that have been allocated. For
    /// failed construction and destruction
    void free_all() {
	if (hstmt_) {
	    SQLFreeHandle(SQL_HANDLE_STMT, hstmt_);
	}
    
	// if (hdbc_) {
	//     SQLDisconnect(hdbc_);
	//     SQLFreeHandle(SQL_HANDLE_DBC, hdbc_);
	// }
	
	// if (henv_) {
	//     SQLFreeHandle(SQL_HANDLE_ENV, henv_);
	// }
    }
    */
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
