#ifndef ROW_BUFFER_HPP
#define ROW_BUFFER_HPP

#include "stmt_handle.hpp"
#include "yaml.hpp"
#include "category.hpp"

/// Holds the column bindings for an in-progress query. Allows
/// rows to be fetched one at a time
class RowBuffer {
public:
    /// Make sure you only do this after executing the stam
    RowBuffer(const std::shared_ptr<StmtHandle> & stmt)
	: stmt_{stmt}
    {
	// Loop over the columns (note: indexed from 1!)
	// Get the column types
	std::size_t num_columns{stmt_->num_columns()};
	
	// Get all the column names. This is where you might do
	// column name remapping.
	for (std::size_t n = 1; n <= num_columns; n++) {
	    std::string colname{stmt_->column_name(n)};
	    col_bindings_.push_back(stmt_->make_binding(n));
	}
    }

    // Get the number of columns
    std::size_t size() const {
	return col_bindings_.size();
    }

    std::vector<std::string> column_names() const {
	std::vector<std::string> names;
	for (const auto & bind : col_bindings_) {
	    names.push_back(bind.col_name());
	}
	return names;
    }

    const std::string & get(std::size_t col_index) const {
	return current_row_[col_index];
    }
    
    /// Fetch the next row of data into an internal state
    /// variable. Use get() to access items from the current
    /// row. This function throws a logic error if there are
    /// not more rows.
    void fetch_next_row() {
	if(not stmt_->fetch()) {
	    throw std::logic_error("No more rows");
	}
	for (auto & bind : col_bindings_) {
	    std::string value;
	    try {
		value = bind.read_buffer();
	    } catch (const std::logic_error &) {
		value = "NULL";
	    }
	    current_row_.push_back(value);
	}
    }
    
private:
    std::shared_ptr<StmtHandle> stmt_;
    std::vector<ColBinding> col_bindings_;
    std::vector<std::string> current_row_;
};

#endif
