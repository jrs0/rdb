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

struct Handle {
    SQLHANDLE handle;
    SQLSMALLINT type;
    Handle(SQLHANDLE handle, SQLSMALLINT type)
	: handle{handle}, type{type} {}
};

/// Test an ODBC return code, return
/// true for success and false for failure
bool result_ok(const Handle & handle, SQLRETURN ret_code) {
    switch(ret_code) {
    case SQL_SUCCESS_WITH_INFO:
	return true;
    case SQL_SUCCESS:
	return true;
    case SQL_INVALID_HANDLE:
	throw std::runtime_error("SQLRETURN Invalid Handle");
    case SQL_NO_DATA_FOUND:
	throw std::runtime_error("SQLRETURN No data found");
    case SQL_ERROR:
	// Throws runtime_error
	handle_diagnostic_record(handle.handle, handle.type, ret_code);
	
    default:
	std::stringstream ss;
	ss << "Unexpected return code in SQLRETURN: " << ret_code; 
	throw std::runtime_error(ss.str());
    }
}

void ok_or_throw(const Handle & handle, SQLRETURN r, const std::string & description) {
    try {
	result_ok(handle, r);
	debug_msg(description);
    } catch (const std::runtime_error & e) {
	std::stringstream ss;
	ss << "'" << description << "' failed (" << e.what() << ")";
	throw std::runtime_error(ss.str());
    }

}

#endif
