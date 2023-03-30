#ifndef ROW_BUFFER_HPP
#define ROW_BUFFER_HPP

#include <string>

template<class T>
concept RowBuffer = requires(T t, const std::string & s)
    {
     t.size();
    };

/// Get a column from the row buffer. Throws out_of_range if
/// the column is not found.
template<typename T>
T column(const std::string & column_name, const RowBuffer auto & row) {
    return row.template at<T>(column_name);
}


#endif
