#ifndef DEBUG_HPP
#define DEBUG_HPP

#include <string>
#include <sstream>
#include <iostream>
#include <stdexcept>

#ifdef _WIN64
#include <windows.h>
#endif

#include <sql.h>
#include <sqlext.h>
#include <memory>

//#define DEBUG

void throw_unimpl_sql_type(const std::string & type);

void debug_msg(const std::string &msg);

// Not currently return the correct messages, but throws runtime_error if
// there is a problem
void handle_diagnostic_record(SQLHANDLE handle, SQLSMALLINT type,
                              RETCODE ret_code);

class Handle {
public:
    Handle(SQLHANDLE handle, SQLSMALLINT type)
	: handle_{handle}, type_{type} {}
    SQLHANDLE handle() const { return handle_; }
    SQLSMALLINT type() const { return type_; }
private:
    SQLHANDLE handle_;
    SQLSMALLINT type_;
};

/// Test an ODBC return code, return
/// true for success and false for failure
bool result_ok(const Handle &handle, SQLRETURN ret_code);

void ok_or_throw(const Handle &handle, SQLRETURN r,
                 const std::string &description);

#endif
