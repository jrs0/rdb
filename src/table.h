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
    struct CallAddRowFirst {};

    /// @brief  Increment the row counter to point to the next row
    void add_row() {
        for (auto & column: columns_) {
            column.second.push_back(0);
        }
        next_row++;
    }

    /// @brief Increment the count in a column at the current row
    /// @param column The column name to increment
    void increment_count(const std::size_t &column_id) {

        if (next_row == 0) {
            throw CallAddRowFirst{};
        }

        try {
            columns_.at(column_id)[next_row-1]++;
        } catch (const std::out_of_range &) {
            columns_[column_id] = std::vector<long long>(next_row);
            columns_[column_id][next_row-1]++;
        }
    }

    /// @brief Get the count columns. Each of these maps a column id
    /// to a vector of values, all of which are the same length
    /// @return A map from column ids to columns
    const auto & columns() const {
        return columns_;
    }

   private:
    std::map<std::size_t, std::vector<long long>> columns_;
    std::size_t next_row{0};
};

#endif