#ifndef ROW_BUFFER_HPP
#define ROW_BUFFER_HPP

#include "sql_types.h"
#include <string>

template<class T>
concept RowBuffer = requires(T t, const std::string & s) {
    t.template at<Varchar>(s);
};

/// Get a column from the row buffer. Throws out_of_range if
/// the column is not found.
template<typename T>
T column(const std::string & column_name, const RowBuffer auto & row) {
    return row.template at<T>(column_name);
}

namespace RowBufferException {

    /// Thrown by the constructor if there are no rows, or by
    /// fetch_next_row if there are no more rows.
    struct NoMoreRows {};

    /// Throw by at() if the columns is not present
    struct ColumnNotFound {};

    /// Thrown if you try to all at() on a column but pass
    /// the wrong column type
    struct WrongColumnType {};

}


#endif
