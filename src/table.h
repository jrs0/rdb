#ifndef Table_HPP
#define TABLE_HPP

#include <map>

/**
 * @brief A dynamically-growing (columns) table of counts
 *
 * The purpose of this class is to store a table of the same type
 * of value in a dense structure, when the number or names of the
 * columns are not known in advance. For example, when generating
 * a table of one-hot encodings of clinical codes, it is not known
 * in advance which codes will come up -- however, python wants
 * the encoding as columns, so this structure adds in empty
 * columns when it comes across now codes.
 *
 * The class is for counting instances of strings. The columns are
 * referred to by string ID (i.e. from the StringLookup).
 *
 */
class CountTable {
   public:
    /// @brief  Increment the row counter to point to the next row
    void next_row() {
        row++;
    }

    /// @brief Increment the count in a column at the current row
    /// @param column The column name to increment
    void increment_count(const std::size_t &column_id) {
        try {
            columns_.at(column_id)[row]++;
        } catch (const std::out_of_range &) {
            columns_[column_id] = std::vector<std::size_t>(row+1);
            columns_[column_id][row]++;
        }
    }

   private:
    std::map<std::size_t, std::vector<std::size_t>> columns_;
    std::size_t row{0};
};

#endif