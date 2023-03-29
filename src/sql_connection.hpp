#ifndef SQL_CONNECTION
#define SQL_CONNECTION

#include <vector>
#include <map>

#include "sql_row_buffer.hpp"
#include "acs.hpp"

/// A simple SQL
class SQLConnection {

public:

    /// Create an SQL connection to a data source
    SQLConnection(const std::string & dsn)
	: dsn_{dsn}, env_{std::make_shared<EnvHandle>()} {
	env_->set_attribute((SQLPOINTER)SQL_OV_ODBC3, 0);
	dbc_ = std::make_shared<ConHandle>(env_, dsn);
	stmt_ = std::make_shared<StmtHandle>(dbc_);
    }

    /// Submit an SQL query and get the result back as a set of
    /// columns (with column names).
    SqlRowBuffer execute_direct(const std::string & query) {
	stmt_->exec_direct(query);
	return SqlRowBuffer{stmt_};
    }
    
private:
    std::string dsn_; ///< Data source name
    std::shared_ptr<EnvHandle> env_; ///< Global environment handle
    std::shared_ptr<ConHandle> dbc_; ///< Connection handle
    std::shared_ptr<StmtHandle> stmt_; ///< Statement handle

};

#endif
