#ifndef DEBUG_HPP
#define DEBUG_HPP

#include <string>
#include <sstream>
#include <iostream>
#include <stdexcept>

#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <memory>

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

#endif
