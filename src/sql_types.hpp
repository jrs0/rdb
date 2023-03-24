#ifndef SQL_TYPES
#define SQL_TYPES

#include <variant>

class Buffer {
public:
    Buffer(std::size_t buffer_length)
	: buffer_length_{buffer_length}, buffer_{new char[buffer_length_]}
    { }
    std::size_t length() const {
	return buffer_length_;
    }
    char* get() {
	return buffer_.get();
    }
private:
    /// The buffer length in bytes
    std::size_t buffer_length_;

    /// The buffer area
    std::unique_ptr<char[]> buffer_;    
};

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

private:
    
};

class IntegerBuffer {
public:
    IntegerBuffer()
	: buffer_{std::make_unique<long>}
    {
	
    }
private:
    std::unique_ptr<long> buffer_;_
};

using ColBufferType = std::variant<VarcharBuffer, IntegerBuffer>;


#endif


