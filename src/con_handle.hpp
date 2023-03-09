#ifndef CON_HANDLE_HPP
#define CON_HANDLE_HPP

#include "env_handle.hpp"

class ConHandle {
public:
    ConHandle(std::shared_ptr<EnvHandle> henv, const std::string & dsn)
	: henv_{henv}
    {
	SQLRETURN r = SQLAllocHandle(SQL_HANDLE_DBC, henv_->get_handle().handle, &hdbc_);
	try {
	    result_ok(henv_->get_handle(), r);
	    debug_msg("Allocated the connection handle");
	} catch (const std::runtime_error & e) {
	    std::stringstream ss;
	    ss << "Failed to alloc connection handle: " << e.what();
	    throw std::runtime_error(ss.str());
	}

	// Make the connection
	r = SQLConnect(hdbc_, (SQLCHAR*)dsn.c_str(), SQL_NTS, NULL, 0, NULL, 0);
	try {
	    result_ok(get_handle(), r);
	    debug_msg("Established connection");
	} catch (const std::runtime_error & e) {
	    free_handle();
	    std::stringstream ss;
	    ss << "Failed to connect to DSN: " << e.what();
	    throw std::runtime_error(ss.str());
	}
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
