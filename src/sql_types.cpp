#include "sql_types.hpp"

std::ostream &operator<<(std::ostream &os, const Integer &integer) {
    integer.print(os);
    return os;
}
