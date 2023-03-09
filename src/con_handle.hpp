#ifndef CON_HANDLE_HPP
#define CON_HANDLE_HPP

#include "env_handle.hpp"

class ConHandle {
public:
    ConHandle(std::shared_ptr<EnvHandle> henv, const std::string & dsn)
	: henv_{henv}
    {
	SQLRETURN r = SQLAllocHandle(SQL_HANDLE_DBC, henv_->get_handle().handle, &hdbc_);
	ok_or_throw(henv_->get_handle(), r, "Allocating the connection handle");

	// Make the connection
	r = SQLConnect(hdbc_, (SQLCHAR*)dsn.c_str(), SQL_NTS, NULL, 0, NULL, 0);
	ok_or_throw(get_handle(), r, "Attempting to connect to the server");
    }
    
    Handle get_handle() {
	return Handle{hdbc_, SQL_HANDLE_DBC};
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

#endif
