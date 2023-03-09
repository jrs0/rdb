#ifndef ENV_HANDLE_HPP
#define ENV_HANDLE_HPP

#include "debug.hpp"

/// Global environment handle
class EnvHandle {
public:
    EnvHandle() {
	SQLRETURN r = SQLAllocHandle(SQL_HANDLE_ENV,
				     SQL_NULL_HANDLE,
				     &henv_);
	try {
	    result_ok(get_handle(), r);
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
	    result_ok(get_handle(), r);
	    debug_msg("Set environment variables");
	} catch (const std::runtime_error & e) {
	    std::stringstream ss;
	    ss << "Failed to set env variable: " << e.what();
	    throw std::runtime_error(ss.str());
	}	
    }
    Handle get_handle() {
	return Handle{henv_, SQL_HANDLE_ENV};
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

#endif
