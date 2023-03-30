#ifndef SQL_ROW_BUFFER_HPP
#define SQL_ROW_BUFFER_HPP

#include <functional>

#include "stmt_handle.hpp"
#include "yaml.hpp"
#include "category.hpp"
#include "random.hpp"
#include "row_buffer.hpp"

/// Holds the column bindings for an in-progress query. Allows
/// rows to be fetched one at a time.
class SqlRowBuffer {
public:
    /// Make sure you only do this after executing the statement
    SqlRowBuffer(const std::shared_ptr<StmtHandle> & stmt)
	: stmt_{stmt}
    {
	// Loop over the columns (note: indexed from 1!)
	// Get the column types
	std::size_t num_columns{stmt_->num_columns()};
	
	// Get all the column names. This is where you might do
	// column name remapping.
	for (std::size_t n = 1; n <= num_columns; n++) {
	    auto column_name{stmt_->column_name(n)};
	    column_buffers_.insert({column_name, stmt_->make_buffer(n)});
	}
    }

    // Get the number of columns
    std::size_t size() const {
	return column_buffers_.size();
    }

    /// Throws out_of_range if column does not exist, and
    /// bad_variant_access if T is not this column's type
    template<typename T>
    T at(std::string column_name) const {
	auto & buffer {
	    std::get<typename T::Buffer>(column_buffers_.at(column_name))
		};
	return buffer.read();
    }
    
    /// Fetch the next row of data into an internal state
    /// variable. Use get() to access items from the current
    /// row. This function throws a logic error if there are
    /// not more rows.
    void fetch_next_row() {
	if(not stmt_->fetch()) {
	    throw std::logic_error("No more rows");
	}
	current_row_++;
    }

    auto current_row_number() const {
	return current_row_;
    }
    
private:
    std::size_t current_row_{0};
    std::shared_ptr<StmtHandle> stmt_;
    std::map<std::string, BufferType> column_buffers_;
};

#endif
