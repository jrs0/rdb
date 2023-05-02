#ifndef CON_HANDLE_HPP
#define CON_HANDLE_HPP

#include <sstream>

#include "env_handle.hpp"
#include "yaml.hpp"

/// Throws runtime error if not present, adds to connection
/// string in correct format
void check_and_append_key(std::stringstream & ss,
			  const std::string & key,
			  const YAML::Node & cred) {
    if(cred[key]) {
	ss << key << "=" << cred[key] << ";";
    } else {
	throw std::runtime_error("Missing '" + key + "' in credentials file");
    }
}

std::string make_connection_string(const YAML::Node & cred) {
    std::stringstream ss;
    check_and_append_key(ss, "driver", cred);
    check_and_append_key(ss, "server", cred);
    check_and_append_key(ss, "uid", cred);
    check_and_append_key(ss, "pwd", cred);

    // For ODBC Driver 18 for SQL Server, SSL is turned on by
    // default (see https://stackoverflow.com/questions/71732117/
    // laravel-sql-server-error-odbc-driver-18-for-sql-serverssl
    // -provider-error141). You 
    check_and_append_key(ss, "TrustServerCertificate", cred);
    return ss.str();
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
	std::cout << "COnnection string: " << con_string << std::endl;
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
