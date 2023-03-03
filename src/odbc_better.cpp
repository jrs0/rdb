#include <string>
#include <iostream>

#include <windows.h>
#include <sql.h>
#include <sqlext.h>

/// Test an ODBC return code, return
/// true for success and false for failure
bool test_result(SQLRETURN ret_code) {
    switch(ret_code) {
    case SQL_SUCCESS_WITH_INFO:
	return true;
    case SQL_SUCCESS:
	return true;
    case SQL_ERROR:
	return false;	
    default:
	std::cerr << "Unexpected return code " << ret_code << std::endl;
	return false;
    }
}

/// A simple SQL
class SQLConnection {

public:
    SQLConnection(const std::string & dsn) : dsn_{dsn} {
	// Allocate global environment handle
	if(test_result(SQLAllocHandle(SQL_HANDLE_ENV,
				      SQL_NULL_HANDLE,
				      &henv_))) {
	    std::cerr << "Failed to alloc ODBC global env handle"
		      << std::endl;
	    exit(-1);
	}

	// Set environment attributes
	if(test_result(SQLSetEnvAttr(henv_,
				     SQL_ATTR_ODBC_VERSION,
				     (SQLPOINTER)SQL_OV_ODBC3,
				     0))) {
	    std::cerr << "Failed to set env attributes ODBC3"
		      << std::endl;
	    exit(-1);
	}
	
	// Allocate a connection
	if(test_result(SQLAllocHandle(SQL_HANDLE_DBC,
				      henv_,
				      &hdbc_))) {
	    std::cerr << "Failed to alloc connection handle"
		      << std::endl;
	    exit(-1);
	}	

	// Make the connection
	if(test_result(SQLConnect(hdbc_,
				  (SQLCHAR*)dsn_.c_str(),
				  dsn_.size(), // Length of server name
				  NULL, 0, NULL, 0))) {
	    std::cerr << "Failed to set env attributes ODBC3"
		      << std::endl;
	    exit(-1);
	}

	
    }

    ~SQLConnection() {
	// Free the global env handle?
	// Free the connection handle?
    }

private:
    std::string dsn_; ///< Data source name
    SQLHENV henv_; ///< Global environment handle
    SQLHDBC hdbc_; ///< Connection handle

};

