#ifndef SQL_TYPES
#define SQL_TYPES

#include <variant>

void throw_unimpl_sql_type(const std::string & type) {
    std::stringstream ss;
    ss << "Type '" << type << "' not yet implemented";
    throw std::runtime_error(ss.str());
}

// Exception thrown on NULL value
struct NullValue{};

class Varchar {
public:
    using Buffer = class VarcharBuffer;
    // Will default construct to a null varchar
    Varchar() = default;
    Varchar(const std::string & string)
	: null_{false}, string_{string} {}
    std::string read() const {
	if (not null_) {
	    return string_;
	} else {
	    throw NullValue{};
	}
    }
    bool null() const { return null_; }
private:
    bool null_{true};
    std::string string_{""};
};

// Will default construct to a null integer
class Integer {
public:
    using Buffer = class IntegerBuffer;
    // Will default construct to a null integer
    Integer() = default;
    Integer(long value) : null_{false}, value_{value} {}
    long read() const {
	if (not null_) {
	    return value_;
	} else {
	    throw NullValue{};
	}
    }
    bool null() const { return null_; }
private:
    bool null_{true};
    long value_{0};
};

using SqlType = std::variant<Varchar, Integer>;

class VarcharBuffer {
public:
    VarcharBuffer(Handle hstmt, std::size_t col_index,
		  std::size_t buffer_length)
	: buffer_length_{buffer_length}, buffer_{new char[buffer_length_]},
	  len_ind_{std::make_unique<SQLLEN>(0)} {
	// Buffer length is in bytes, but the column_length might be in chars
	// Here, the type is specified in the SQL_C_CHAR position.
	SQLRETURN r = SQLBindCol(hstmt.handle(), col_index, SQL_C_CHAR,
				 (SQLPOINTER)buffer_.get(),
				 buffer_length_, len_ind_.get());
	ok_or_throw(hstmt, r, "Binding varchar column");
    }

    Varchar read() const {
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

private:
    /// The buffer length in bytes
    std::size_t buffer_length_;

    /// The buffer area
    std::unique_ptr<char[]> buffer_;    
    
    /// Where the output data length with
    /// be written
    std::unique_ptr<SQLLEN> len_ind_;
};

class IntegerBuffer {
public:
    IntegerBuffer(Handle hstmt, std::size_t col_index)
	: buffer_{std::make_unique<long>(0)},
	  len_ind_{std::make_unique<SQLLEN>(0)} {
	// Note: because integer is a fixed length type, the buffer length
	// field is ignored. 
	SQLRETURN r = SQLBindCol(hstmt.handle(), col_index, SQL_C_LONG,
				 (SQLPOINTER)buffer_.get(), 0,
				 len_ind_.get());
	ok_or_throw(hstmt, r, "Binding integer column");
    }

    Integer read() const {
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

private:
    std::unique_ptr<long> buffer_;
    
    /// Where the output data length with
    /// be written
    std::unique_ptr<SQLLEN> len_ind_;    
};

using BufferType = std::variant<VarcharBuffer, IntegerBuffer>;

#endif


