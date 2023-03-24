#ifndef SQL_TYPES
#define SQL_TYPES

#include <variant>

class Varchar {
public:
    
private:
    bool null_;
    std::string string_;
};


class Integer {
public:
    Integer() {}
private:
    bool null_;
    long value_;
};

using SqlType = std::variant<Varchar, Datetime>;

class VarcharBuffer {
public:
    VarcharBuffer(Handle hstmt, const std::string & col_name,
		  std::size_t col_index, std::size_t buffer_len)
	: buffer_len_{buffer_len}, buffer_{new char[buffer_len_]}
	  col_index_{col_index}, col_name_{col_name},  col_type_{col_type},
	  len_ind_{std::make_unique<SQLLEN>(0)} {
	// Buffer length is in bytes, but the column_length might be in chars
	// Here, the type is specified in the SQL_C_CHAR position.
	SQLRETURN r = SQLBindCol(hstmt.handle(), col_index_, SQL_C_CHAR,
				 (SQLPOINTER)buffer_.get(),
				 buffer_.length(), len_ind_.get());
	ok_or_throw(hstmt, r, "Binding varchar column");
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
    IntegerBuffer()
	: buffer_{std::make_unique<long>}
    {
	
    }
private:
    std::unique_ptr<long> buffer_;
};

using ColBufferType = std::variant<VarcharBuffer, IntegerBuffer>;


#endif


