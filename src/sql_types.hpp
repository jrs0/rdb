#ifndef SQL_TYPES
#define SQL_TYPES

#include <variant>
#include <ctime>
#include <iomanip>

#ifdef _WIN64
#include <windows.h>
#endif

#include <sql.h>
#include <sqlext.h>
#include "sql_debug.hpp"

// Could not determine the returned data length
struct SqlNoTotal{};

class Varchar {
public:
    struct Null {};
    using Buffer = class VarcharBuffer;
    // Will default construct to a null varchar
    Varchar() = default;
    Varchar(const std::string & value)
	: null_{false}, value_{value} {}
    std::string read() const {
	if (not null_) {
	    return value_;
	} else {
	    throw Null{};
	}
    }
    void print(std::ostream & os = std::cout) const {
	os << "Integer: ";
	if (null_) {
	    os << "NULL" << std::endl;
	} else {
	    os << value_ << std::endl;	    
	}
    }
    bool null() const { return null_; }
private:
    bool null_{true};
    std::string value_{""};
};

// Will default construct to a null integer
class Integer {
public:
    struct Null {};
    using Buffer = class IntegerBuffer;
    // Will default construct to a null integer
    Integer() = default;
    Integer(unsigned long long value) : null_{false}, value_{value} {}
    unsigned long long read() const {
	if (not null_) {
	    return value_;
	} else {
	    throw Null{};
	}
    }
    void print(std::ostream & os = std::cout) const {
	os << "Integer: ";
	if (null_) {
	    os << "NULL";
	} else {
	    os << value_;	    
	}
    }
    bool null() const { return null_; }
private:
    bool null_{true};
    unsigned long long value_{0};
};

std::ostream &operator<<(std::ostream &os, const Integer &integer);

// Stores an absolute time as a unix timestamp, constructed from
// date components assuming that BST may be in effect.
class Timestamp {
public:
    struct Null {};
    using Buffer = class TimestampBuffer;

    // Will default construct to a null timestamp
    Timestamp() = default;
    Timestamp(unsigned long long timestamp)
	: null_{false}, unix_timestamp_{timestamp} { }
    Timestamp(const SQL_TIMESTAMP_STRUCT & datetime)
	: null_{false} {

	std::tm tm;
	
	// Set the year field
	if (datetime.year < 1900) {
	    throw std::runtime_error("Encountered invalid year for unix "
				     "timestamp comversion (before 1900)");
	} else {
	    tm.tm_year = datetime.year - 1900;
	}

	// // Set the month
	if ((datetime.month > 0) and (datetime.month < 13)) {
	    tm.tm_mon = datetime.month - 1;
	} else {
	    throw std::runtime_error("Encountered invalid month for unix "
				     "timestamp comversion");
	}
	
	tm.tm_mday = datetime.day;
	tm.tm_hour = datetime.hour;
	tm.tm_min = datetime.minute;
	tm.tm_sec = datetime.second;

	// Pass 0 to assume that the timestamp is recorded in the UTC
	// (GMT) timezone
	//tm.tm_isdst = 0;
	
	// Pass -1 to assume that the the times are recorded according
	// to the wall clock in the UK. This is most likely what is
	// being recorded in the HES data. This appears to match the
	// default assumption of POSIXct, which assumes BST. This is
	// a valid interpretation of the database times if they are
	// wall-clock time as recorded in the UK.
	tm.tm_isdst = -1;
	    
	// Convert to timestamp
	unix_timestamp_
	    = static_cast<unsigned long long>(std::mktime(&tm));
    }

    friend auto operator<=>(const Timestamp &, const Timestamp &) = default;
    friend bool operator==(const Timestamp &, const Timestamp &) = default;
    
    unsigned long long read() const {
	if (not null_) {
	    return unix_timestamp_;
	} else {
	    throw Null{};
	}
    }
    void print(std::ostream & os = std::cout) const {
	if (null_) {
	    os << "NULL";
	} else {
	    auto t{static_cast<std::time_t>(unix_timestamp_)};
	    os << std::put_time(std::localtime(&t), "%F %T");
	}
    }
    bool null() const { return null_; }
private:
    bool null_{true};
    unsigned long long unix_timestamp_{0};
};

std::ostream &operator<<(std::ostream &os, const Timestamp &timestamp);

template<std::integral T>
Timestamp operator+(const Timestamp & time, T offset_seconds) {
    return Timestamp{time.read() + offset_seconds};
}

using SqlType = std::variant<Varchar,
			     Integer,
			     Timestamp>;

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

class IntegerBuffer {
public:
    IntegerBuffer(Handle hstmt, std::size_t col_index)
	: buffer_{std::make_unique<unsigned long long>(0)},
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
	    if (*data_size_ != sizeof(unsigned long long)) {
		throw std::runtime_error("Fixed type size not equal to C "
					 "type. Returned size = "
					 + std::to_string(*data_size_) +
					 " but size of long = "
					 + std::to_string(sizeof(unsigned long long)));
	    }
	    return Integer{*buffer_};
	}
    }

private:
    std::unique_ptr<unsigned long long> buffer_;
    
    /// For a fixed size type, this is the size of the
    /// type written by the driver. Must be less than
    /// or equal to the buffer size to avoid memory errors.
    std::unique_ptr<SQLLEN> data_size_;
};

class TimestampBuffer {
public:
    TimestampBuffer(Handle hstmt, std::size_t col_index)
	: buffer_{std::make_unique<SQL_TIMESTAMP_STRUCT>()},
	  data_size_{std::make_unique<SQLLEN>(0)} {
	// Note: because integer is a fixed length type, the buffer length
	// field is ignored. 
	SQLRETURN r = SQLBindCol(hstmt.handle(), col_index,
				 SQL_C_TYPE_TIMESTAMP,
				 (SQLPOINTER)buffer_.get(), 0,
				 data_size_.get());
	ok_or_throw(hstmt, r, "Binding timestamp");
    }
    
    Timestamp read() const {
	switch (*data_size_) {
	case SQL_NULL_DATA:
	    return Timestamp{};
	default:
	    if (*data_size_ != sizeof(SQL_TIMESTAMP_STRUCT)) {
		throw std::runtime_error("Fixed type size not equal to C "
					 "type. Returned size = "
					 + std::to_string(*data_size_) +
					 " but size of DATETIME = "
					 + std::to_string(sizeof(SQL_TIMESTAMP_STRUCT)));
	    }

	    // Convert datetime fields to unix timestamp here
	    
	    return Timestamp{*buffer_};
	}
    }

private:
    std::unique_ptr<SQL_TIMESTAMP_STRUCT> buffer_;
    
    /// For a fixed size type, this is the size of the
    /// type written by the driver. Must be less than
    /// or equal to the buffer size to avoid memory errors.
    std::unique_ptr<SQLLEN> data_size_;
};

using BufferType = std::variant<VarcharBuffer,
				IntegerBuffer,
				TimestampBuffer>;

#endif


