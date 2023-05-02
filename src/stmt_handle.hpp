#ifndef STMT_HANDLE_HPP
#define STMT_HANDLE_HPP

#include "con_handle.hpp"
#include "sql_types.hpp"


/// Make a column binding for a VARCHAR column
BufferType make_varchar_binding(std::size_t index,
				Handle hstmt) {
    
    /// Get length of the character
    std::size_t varchar_length{0};
    SQLRETURN r = SQLColAttribute(hstmt.handle(), index, SQL_DESC_LENGTH,
				  NULL, 0, NULL, (SQLLEN*)&varchar_length);
    ok_or_throw(hstmt, r, "Getting column type length attribute");

    /// Pass SQL_C_CHAR type for VARCHAR
    return VarcharBuffer{hstmt, index, varchar_length};
}

/// Make a column binding for an INTEGER column
BufferType make_integer_binding(std::size_t index,
				 Handle hstmt) {
    
    /// Use SQL_C_LONG
    return IntegerBuffer{hstmt, index};
}

/// Make a column binding for a date/datetime/timestamp column
BufferType make_timestamp_binding(std::size_t index,
				  Handle hstmt) {
    
    /// Use SQL_C_LONG
    return TimestampBuffer{hstmt, index};
}


class StmtHandle {
public:
    StmtHandle(std::shared_ptr<ConHandle> hdbc)
	: hdbc_{hdbc}
    {
	SQLRETURN r = SQLAllocHandle(SQL_HANDLE_STMT,
				     hdbc_->get_handle().handle(), &hstmt_);
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
    BufferType make_buffer(std::size_t index) {

	std::string col_name{column_name(index)};
	
	/// Get the column type
	SQLLEN type{column_type(index)};
	switch (type) {
	case SQL_VARCHAR:
	    /// Store a varchar in a std::string. Convert to
	    /// a char string
	    return make_varchar_binding(index, get_handle());
	    
	case SQL_INTEGER:
	    // 32-bit signed or unsigned integer -> map to SqlInteger
	    // Map
	    //target_type =
	    return make_integer_binding(index, get_handle());    
	case SQL_BIGINT:
	    // 64-bit signed or unsigned int -> map to SqlInteger
	    return make_integer_binding(index, get_handle());
	    
	case SQL_TYPE_TIMESTAMP:
	    // Year, month, day, hour, minute, and second
	    // -> map to SqlDatetime
	    return make_timestamp_binding(index, get_handle());
	    break;
	    
	case SQL_TYPE_DATE:
	    return make_timestamp_binding(index, get_handle());
	    break;
	    
	default: {
	    throw_unimpl_sql_type("Unknown: " + std::to_string(type));
	    // To remove compiler warning "control reaches end of non-void function"
	    throw std::runtime_error("Not expecting to get here in make_buffer()");
	}
	}	
    }

    /// Fetch a single row into the column bindings. Returns false
    /// if no data was returned.
    bool fetch() {
	// Run one fetch (get a single row)
	SQLRETURN r = SQLFetch(hstmt_);
	///TODO this is a bug, what if r is a *real* error -- need
	// to catch and throw that.
	return (r != SQL_NO_DATA_FOUND);
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
