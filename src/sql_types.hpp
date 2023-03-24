#ifndef SQL_TYPES
#define SQL_TYPES

#include <variant>

void throw_unimpl_sql_type(const std::string & type) {
    std::stringstream ss;
    ss << "Type '" << type << "' not yet implemented";
    throw std::runtime_error(ss.str());
}

class Varchar {
public:
    // Will default construct to a null varchar
    Varchar() = default;
    Varchar(const std::string & string)
	: null_{false}, string_{string} {}
private:
    bool null_{true};
    std::string string_{""};
};

// Will default construct to a null integer
class Integer {
public:
    // Will default construct to a null integer
    Integer() = default;
    Integer(long value) : null_{false}, value_{value} {}
private:
    bool null_{true};
    long value_{0};
};

using SqlType = std::variant<Varchar, Integer>;

class VarcharBuffer {
public:
    VarcharBuffer(Handle hstmt, const std::string & col_name,
		  std::size_t col_index, std::size_t buffer_length)
	: buffer_length_{buffer_length}, buffer_{new char[buffer_length_]},
	  col_name_{col_name}, len_ind_{std::make_unique<SQLLEN>(0)} {
	// Buffer length is in bytes, but the column_length might be in chars
	// Here, the type is specified in the SQL_C_CHAR position.
	SQLRETURN r = SQLBindCol(hstmt.handle(), col_index, SQL_C_CHAR,
				 (SQLPOINTER)buffer_.get(),
				 buffer_length_, len_ind_.get());
	ok_or_throw(hstmt, r, "Binding varchar column");
    }

    Varchar read() {
	switch (*len_ind_) {
	case SQL_NO_TOTAL:
	    throw_unimpl_sql_type("SQL_NO_TOTAL");
	    break;
	case SQL_NULL_DATA:
	    return Varchar{};
	default:
	    return Varchar{buffer_.get()};
	}
    }

    std::string col_name() const {
	return col_name_;
    }

private:
    /// The buffer length in bytes
    std::size_t buffer_length_;

    /// The buffer area
    std::unique_ptr<char[]> buffer_;    

    std::string col_name_;
    
    /// Where the output data length with
    /// be written
    std::unique_ptr<SQLLEN> len_ind_;
};

class IntegerBuffer {
public:
    IntegerBuffer(Handle hstmt, const std::string & col_name,
		  std::size_t col_index)
	: buffer_{std::make_unique<long>(0)}, col_name_{col_name},
	  len_ind_{std::make_unique<SQLLEN>(0)} {
	// Note: because integer is a fixed length type, the buffer length
	// field is ignored. 
	SQLRETURN r = SQLBindCol(hstmt.handle(), col_index, SQL_C_LONG,
				 (SQLPOINTER)buffer_.get(), 0,
				 len_ind_.get());
	ok_or_throw(hstmt, r, "Binding integer column");
    }

    Integer read() {
	switch (*len_ind_) {
	case SQL_NO_TOTAL:
	    throw_unimpl_sql_type("SQL_NO_TOTAL");
	    break;
	case SQL_NULL_DATA:
	    return Integer{};
	default:
	    return Integer{*buffer_};
	}
    }

    std::string col_name() const {
	return col_name_;
    }

    
private:
    std::unique_ptr<long> buffer_;

    std::string col_name_;
    
    /// Where the output data length with
    /// be written
    std::unique_ptr<SQLLEN> len_ind_;    
};

using ColBinding = std::variant<VarcharBuffer, IntegerBuffer>;


#endif


