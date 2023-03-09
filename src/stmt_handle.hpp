#ifndef STMT_HANDLE_HPP
#define STMT_HANDLE_HPP

#include "con_handle.hpp"


void throw_unimpl_sql_type(const std::string & type) {
    std::stringstream ss;
    ss << "Type '" << type << "' not yet implemented";
    throw std::runtime_error(ss.str());
}

/// 
class ColBinding {
public:
    ColBinding(Handle hstmt, const std::string & name, SQLLEN type,
	       std::size_t col_index, std::size_t buffer_size)
	: name_{name}, type_{type},
	  col_index_{col_index},
	  buffer_size_{buffer_size},
	  return_size_{std::make_unique<SQLLEN>(0)},
	  buffer_{new char[buffer_size]}
    {
	// What type to pass instead of SQL_C_TCHAR?
	// Buffer length is in bytes, but the column_length might be in chars
	SQLRETURN r = SQLBindCol(hstmt.handle, col_index, SQL_C_TCHAR,
				 (SQLPOINTER)buffer_.get(),
				 buffer_size_, return_size_.get());
	try {
	    result_ok(hstmt, r);
	} catch (std::runtime_error & e) {
	    std::stringstream ss;
	    ss << "Failed to bind columns " << col_index << ":" << e.what();
	    throw std::runtime_error(ss.str());
	}
	debug_msg("Bound column");
    }

    void print_type() {
	switch (type_) {
	case SQL_CHAR:
	    throw_unimpl_sql_type("SQL_CHAR");
	    break;
	case SQL_VARCHAR:
	    ///
	    std::cout << "SQL_VARCHAR";
	    break;
	case SQL_LONGVARCHAR:
	    throw_unimpl_sql_type("SQL_LONGVARCHAR");
	    break;
	case SQL_WCHAR:
	    throw_unimpl_sql_type("SQL_WCHAR");
	    break;
	case SQL_WVARCHAR:
	    throw_unimpl_sql_type("SQL_WVARCHAR");
	    break;
	case SQL_WLONGVARCHAR:
	    throw_unimpl_sql_type("SQL_WLONGVARCHAR");
	    break;

	case SQL_DECIMAL:
	    throw_unimpl_sql_type("SQL_DECIMAL");
	    break;
	case SQL_NUMERIC:
	    throw_unimpl_sql_type("SQL_NUMERIC");
	    break;
	case SQL_SMALLINT:
	    throw_unimpl_sql_type("SQL_SMALLINT");
	    break;
	case SQL_INTEGER:
	    std::cout << "SQL_INTEGER";
	    break;

	case SQL_REAL:
	    throw_unimpl_sql_type("SQL_REAL");
	    break;
	case SQL_FLOAT:
	    throw_unimpl_sql_type("SQL_FLOAT");
	    break;
	case SQL_DOUBLE:
	    throw_unimpl_sql_type("SQL_DOUBLE");
	    break;

	case SQL_BIT:
	    throw_unimpl_sql_type("SQL_BIT");
	    break;
	case SQL_BIGINT:
	    std::cout << "SQL_BIGINT";
	    break;
	case SQL_BINARY:
	    throw_unimpl_sql_type("SQL_BINARY");
	    break;
	case SQL_VARBINARY:
	    throw_unimpl_sql_type("SQL_VARBINARY");
	    break;
	case SQL_LONGVARBINARY:
	    throw_unimpl_sql_type("SQL_LONGVARBINARY");
	    break;

	case SQL_TYPE_DATE:
	    throw_unimpl_sql_type("SQL_TYPE_DATE");
	    break;
	case SQL_TYPE_TIME:
	    throw_unimpl_sql_type("SQL_TYPE_TIME");
	    break;
	case SQL_TYPE_TIMESTAMP:
	    std::cout << "SQL_TYPE_TIMESTAMP";
	    break;
	    
	default: {
	    throw_unimpl_sql_type("Unknown: " + std::to_string(type_));	    
	}
	}
    }
    
    void print() {

	std::cout << name_ << " (";
	print_type();
	std::cout << ") ";
	switch (*return_size_) {
	case SQL_NO_TOTAL:
	    throw_unimpl_sql_type("SQL_NO_TOTAL");
	    break;
	case SQL_NULL_DATA:
	    std::cout << "NULL";
	    break;
	default: {
	    // Length of data returned
	    std::string data{buffer_.get()};
	    std::cout << data << "( " << *return_size_ << " bytes)";
	}
	}
	std::cout << std::endl;
    }
    
private:
    std::string name_;
    SQLLEN type_;
    std::size_t col_index_;
    std::size_t buffer_size_;
    std::unique_ptr<SQLLEN> return_size_;
    std::unique_ptr<char[]> buffer_;
};

class StmtHandle {
public:
    StmtHandle(std::shared_ptr<ConHandle> hdbc)
	: hdbc_{hdbc}
    {
	SQLRETURN r = SQLAllocHandle(SQL_HANDLE_STMT, hdbc_->get_handle().handle, &hstmt_);
	ok_or_throw(hdbc_->get_handle(), r, "Allocating statement handle");
    }
    Handle get_handle() {
	return Handle{hstmt_, SQL_HANDLE_STMT};
    }
    void exec_direct(const std::string & query) {
    	// SQL_NTS for null-terminated string (query)
	SQLRETURN r = SQLExecDirect(hstmt_, (SQLCHAR*)query.c_str(), SQL_NTS);
	ok_or_throw(get_handle(), r, "Adding query for direct execution");
    }
    /// Get the number of returned columns (sql_direct comes first)
    std::size_t num_columns() {
	// Obtain the results
	SQLSMALLINT num_columns = 0;
	SQLRETURN r = SQLNumResultCols(hstmt_, &num_columns);
	ok_or_throw(get_handle(), r, "Getting the number of returned rows");
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
	ok_or_throw(get_handle(), r, "Getting column name length attribute");
	
	// Get the column name itself
#define COLUMN_NAME_BUF_LEN 100
	SQLCHAR column_name_buf[COLUMN_NAME_BUF_LEN];
	r = SQLColAttribute(hstmt_, index, SQL_DESC_NAME,
			    column_name_buf, COLUMN_NAME_BUF_LEN,
			    NULL, NULL);
	ok_or_throw(get_handle(), r, "Getting column name attribute");
	std::string column_name((char*)column_name_buf);
	return std::string((char*)column_name_buf);
    }

    SQLLEN column_type(std::size_t index) {
	/// Get column type
	SQLLEN column_type{0};
	SQLRETURN r = SQLColAttribute(hstmt_, index, SQL_DESC_CONCISE_TYPE,
				      NULL, 0, NULL, &column_type);
	ok_or_throw(get_handle(), r, "Getting column type attribute");
	
	return column_type;
    }

    /// Binding to column index (numbered from 1)
    ColBinding make_binding(std::size_t index) {
	
	/// Get the column type
	SQLLEN type = column_type(index);
	
	/// Get length of data type
	SQLLEN column_length{0};
	SQLRETURN r = SQLColAttribute(hstmt_, index, SQL_DESC_LENGTH,
				      NULL, 0, NULL, &column_length);
	ok_or_throw(get_handle(), r, "Getting column type length attribute");
	
	std::cout << "Column " << index << " length is " << column_length << std::endl;

	std::string col_name{column_name(index)};
	ColBinding col_binding{get_handle(), col_name, type, index, column_length};

	return col_binding;
	
    }

    /// Fetch a single row into the column bindings
    void fetch() {
	// Run one fetch (get a single row)
	SQLRETURN r = SQLFetch(hstmt_);
	ok_or_throw(get_handle(), r, "Fetching row");
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
