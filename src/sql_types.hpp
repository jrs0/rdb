#ifndef SQL_TYPES
#define SQL_TYPES

#include <variant>

class Varchar {
public:
    
private:
    bool null_;
    std::string string_;
};


class Datetime {
public:
    SqlDatetime() {}
private:
    bool null_;
    unsigned long unix_time_;
};


using SqlType = std::variant<Varchar, Datetime>;



#endif


