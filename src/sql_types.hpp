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

// Could not determine the returned data length
struct SqlNoTotal{};

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

template<std::integral T>
struct IntegerSqlType;

template<>
struct IntegerSqlType<std::int64_t> {
    static constexpr c_type = SQL_C_UBIGINT;
};

template<>
struct IntegerSqlType<std::int64_t> {
    static constexpr c_type = SQL_C_UBIGINT;
};


// Will default construct to a null integer
template<std::integral T>
class Integer {
public:
    using Buffer = class IntegerBuffer<T>;
    // Will default construct to a null integer
    Integer() = default;
    Integer(T value) : null_{false}, value_{value} {}
    T read() const {
	if (not null_) {
	    return value_;
	} else {
	    throw NullValue{};
	}
    }
    bool null() const { return null_; }
private:
    bool null_{true};
    T value_{0};
};

class VarcharBuffer {
public:
    VarcharBuffer(Handle hstmt, std::size_t col_index,
		  std::size_t buffer_length)
	: buffer_length_{buffer_length}, buffer_{new char[buffer_length_]},
	  data_length_{std::make_unique<SQLLEN>(0)} {
	// Buffer length is in bytes, but the column_length might be in chars
	// Here, the type is specified in the SQL_C_CHAR position.
	SQLRETURN r = SQLBindCol(hstmt.handle(), col_index, SQL_C_CHAR,
				 (SQLPOINTER)buffer_.get(),
				 buffer_length_, data_length_.get());
	ok_or_throw(hstmt, r, "Binding varchar column");
    }

    Varchar read() const {
	switch (*data_length_) {
	case SQL_NO_TOTAL:
	    /// Could not determine the data length
	    /// after conversion
	    throw SqlNoTotal{};
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
    
    /// The length of the data, in bytes, after conversion
    /// but before truncation to the buffer length
    std::unique_ptr<SQLLEN> data_length_;
};

/// Thrown when the driver writes a fixed size type
/// that is too long
struct FixedSizeOverrun {};

template<std::integral T>
class IntegerBuffer {
public:
    IntegerBuffer(Handle hstmt, std::size_t col_index)
	: buffer_{std::make_unique<T>(0)},
	  data_size_{std::make_unique<SQLLEN>(0)} {
	// Note: because integer is a fixed length type, the buffer length
	// field is ignored. 
	SQLRETURN r = SQLBindCol(hstmt.handle(), col_index, SQL_C_UBIGINT,
				 (SQLPOINTER)buffer_.get(), 0,
				 data_size_.get());
	ok_or_throw(hstmt, r, "Binding integer column");
    }
    
    Integer read() const {
	switch (*data_size_) {
	case SQL_NULL_DATA:
	    return Integer{};
	default:
	    if (*data_size_ != sizeof(T)) {
		throw std::runtime_error("Fixed type size not equal to C "
					 "type. Returned size = "
					 + std::to_string(*data_size_) +
					 " but size of C type = "
					 + std::to_string(sizeof(T)));
	    }
	    return Integer{*buffer_};
	}
    }

private:
    std::unique_ptr<T> buffer_;
    
    /// For a fixed size type, this is the size of the
    /// type written by the driver. Must be less than
    /// or equal to the buffer size to avoid memory errors.
    std::unique_ptr<SQLLEN> data_size_;
};

using SqlType = std::variant<Varchar, Integer>;
using BufferType = std::variant<VarcharBuffer, IntegerBuffer>;

#endif


