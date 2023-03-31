#ifndef SQL_CONNECTION
#define SQL_CONNECTION

#include <vector>
#include <map>

#include <string>
#include <memory>
#include "stmt_handle.hpp"
#include "sql_row_buffer.hpp"

/// A simple SQL
class SQLConnection {

public:

    /// Create an SQL connection to a data source
    SQLConnection(const std::string & dsn)
	: env_{std::make_shared<EnvHandle>()} {
	env_->set_attribute((SQLPOINTER)SQL_OV_ODBC3, 0);
	dbc_ = std::make_shared<ConHandle>(env_, dsn);
	stmt_ = std::make_shared<StmtHandle>(dbc_);
    }

    /// Create an SQL connection from raw credentials
    SQLConnection(const YAML::Node & cred)
	: env_{std::make_shared<EnvHandle>()} {

	env_->set_attribute((SQLPOINTER)SQL_OV_ODBC3, 0);
        dbc_ = std::make_shared<ConHandle>(env_, cred);
	stmt_ = std::make_shared<StmtHandle>(dbc_);
    }
    
    /// Submit an SQL query and get the result back as a set of
    /// columns (with column names).
    SqlRowBuffer execute_direct(const std::string & query) {
	stmt_->exec_direct(query);
	return SqlRowBuffer{stmt_};
    }
    
private:
    std::shared_ptr<EnvHandle> env_; ///< Global environment handle
    std::shared_ptr<ConHandle> dbc_; ///< Connection handle
    std::shared_ptr<StmtHandle> stmt_; ///< Statement handle

};

/// Make a connection from the "connection" block (passed as
/// argument), which has either "dsn" (preferred) or "cred"
/// (a path to a credentials file)
SQLConnection new_sql_connection(const YAML::Node & config) {
    if (config["dsn"]) {
	return SQLConnection{config["dsn"].as<std::string>()};
    } else {
	return SQLConnection{config["cred"]};
    }
}

#endif
