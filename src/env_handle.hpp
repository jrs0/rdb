#ifndef ENV_HANDLE_HPP
#define ENV_HANDLE_HPP

#include "sql_debug.hpp"

/// Global environment handle
class EnvHandle {
public:
    EnvHandle() {
	SQLRETURN r = SQLAllocHandle(SQL_HANDLE_ENV,
				     SQL_NULL_HANDLE,
				     &henv_);
	ok_or_throw(get_handle(), r, "Allocating global env");
    }
    void set_attribute(SQLPOINTER value_ptr, SQLINTEGER str_len) {
	SQLRETURN r = SQLSetEnvAttr(henv_, SQL_ATTR_ODBC_VERSION,
				    value_ptr, str_len);
	ok_or_throw(get_handle(), r, "Setting environment variable");
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
