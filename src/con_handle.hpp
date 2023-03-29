#ifndef CON_HANDLE_HPP
#define CON_HANDLE_HPP

#include <sstream>

#include "env_handle.hpp"
#include "yaml.hpp"

std::string make_connection_string(const YAML::Node & cred) {
    std::stringstream con_string;
    if(cred["driver"]) {
	con_string << "driver:" << cred["driver"] << ";";
    } else {
	throw std::runtime_error("Missing 'driver' in credentials file");
    }
    if(cred["server"]) {
	con_string << "server:" << cred["server"] << ";";
    } else {
	throw std::runtime_error("Missing 'server' in credentials file");
    }
    if(cred["uid"]) {
	con_string << "uid:" << cred["uid"] << ";";
    } else {
	throw std::runtime_error("Missing 'uid' in credentials file");
    }
    if(cred["pwd"]) {
	con_string << "pwd:" << cred["pwd"] << ";";
    } else {
	throw std::runtime_error("Missing 'pwd' in credentials file");
    }
    return con_string.str();
}

class ConHandle {
public:
    ConHandle(std::shared_ptr<EnvHandle> henv, const std::string & dsn)
	: henv_{henv}
    {
	SQLRETURN r = SQLAllocHandle(SQL_HANDLE_DBC,
				     henv_->get_handle().handle(), &hdbc_);
	ok_or_throw(henv_->get_handle(), r, "Allocating the connection handle");

	// Make the connection
	r = SQLConnect(hdbc_, (SQLCHAR*)dsn.c_str(), SQL_NTS, NULL, 0, NULL, 0);
	ok_or_throw(get_handle(), r, "Attempting to connect to the server");
    }

    ConHandle(std::shared_ptr<EnvHandle> henv, const YAML::Node & cred)
	: henv_{henv}
    {
	SQLRETURN r = SQLAllocHandle(SQL_HANDLE_DBC,
				     henv_->get_handle().handle(), &hdbc_);
	ok_or_throw(henv_->get_handle(), r, "Allocating the connection handle");

	// Get the connection information
	auto con_string{make_connection_string(cred)};

	r = SQLDriverConnect(hdbc_, NULL, (SQLCHAR*)con_string.c_str(), SQL_NTS,
			     NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
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
