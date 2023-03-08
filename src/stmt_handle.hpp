#ifndef STMT_HANDLE_HPP
#define STMT_HANDLE_HPP

#include "con_handle.hpp"

class StmtHandle {
public:
    StmtHandle(std::shared_ptr<ConHandle> hdbc)
	: hdbc_{hdbc}
    {
	SQLRETURN r = SQLAllocHandle(SQL_HANDLE_STMT, hdbc_->get_handle(), &hstmt_);
	try {
	    result_ok(hdbc_->get_handle(), SQL_HANDLE_DBC, r);
	    debug_msg("Allocated statement handle");
	} catch (const std::runtime_error & e) {
	    std::stringstream ss;
	    ss << "Failed to alloc statement handle: " << e.what();
	    throw std::runtime_error(ss.str());
	}

    }
    SQLHSTMT get_handle() {
	return hstmt_;
    }
    void exec_direct(const std::string & query) {
    	// SQL_NTS for null-terminated string (query)
	SQLRETURN r = SQLExecDirect(hstmt_, (SQLCHAR*)query.c_str(), SQL_NTS);
	try {
	    result_ok(hstmt_, SQL_HANDLE_STMT, r);
	} catch (std::runtime_error & e) {
	    std::stringstream ss;
	    ss << "Failed to add query for direct execution: " << e.what();
	    throw std::runtime_error(ss.str());
	}
	debug_msg("Added statement for execution");
    }
    /// Get the number of returned columns (sql_direct comes first)
    std::size_t num_columns() {
	// Obtain the results
	SQLSMALLINT num_columns = 0;
	SQLRETURN r = SQLNumResultCols(hstmt_, &num_columns);
	try {
	    result_ok(hstmt_, SQL_HANDLE_STMT, r);
	} catch (std::runtime_error & e) {
	    std::stringstream ss;
	    ss << "Failed to get number of returned columns: " << e.what();
	    throw std::runtime_error(ss.str());
	}
	return num_columns;
    }

    /// Remember columns are indexed from 1
    std::string column_name(std::size_t index) {
	if (index == 0) {
	    throw std::runtime_error("Column index 0 out of range");
	}
	SQLSMALLINT column_name_length = 0;
	SQLRETURN r = SQLColAttribute(hstmt_, index, SQL_DESC_NAME, NULL, 0,
			    &column_name_length, NULL);
	try {
	    result_ok(hstmt_, SQL_HANDLE_STMT, r);
	} catch (std::runtime_error & e) {
	    std::stringstream ss;
	    ss << "Failed to get the length of column "
	       << index << ": " << e.what();
	    throw std::runtime_error(ss.str());
	}

	// Get the column name itself
#define COLUMN_NAME_BUF_LEN 100
	SQLCHAR column_name_buf[COLUMN_NAME_BUF_LEN];
	r = SQLColAttribute(hstmt_, index, SQL_DESC_NAME,
			    column_name_buf, COLUMN_NAME_BUF_LEN,
			    NULL, NULL);
	std::string column_name((char*)column_name_buf);
	try {
	    result_ok(hstmt_, SQL_HANDLE_STMT, r);
	} catch (std::runtime_error & e) {
	    std::stringstream ss;
	    ss << "Failed to get column name for col "
	       << index << ": " << e.what();
	    throw std::runtime_error(ss.str());
	}
	return std::string((char*)column_name_buf);
    }
    
    ~StmtHandle() {
	debug_msg("Freeing statement handle");
	if (hstmt_) {
	    SQLFreeHandle(SQL_HANDLE_STMT, hstmt_);
	}
    }
    StmtHandle(const StmtHandle&) = delete;
    StmtHandle& operator=(const StmtHandle&) = delete;
private:
    std::shared_ptr<ConHandle> hdbc_; /// Keep alive for this
    SQLHSTMT hstmt_; ///< Statement handle
};

#endif
