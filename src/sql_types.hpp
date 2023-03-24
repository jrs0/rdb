#ifndef SQL_TYPES
#define SQL_TYPES

#include <variant>

class SqlVarchar {
public:
    
private:
    bool null_;
    std::string string_;
};


class SqlDatetime {
public:
    SqlDatetime() {}
private:
    bool null_;
    unsigned long unix_time_;
};


using SqlType = std::variant<SqlVarchar, SqlDatetime>;
    


#endif


