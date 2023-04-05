#include "sql_types.hpp"

std::ostream &operator<<(std::ostream &os, const Integer &integer) {
    integer.print(os);
    return os;
}

std::ostream &operator<<(std::ostream &os, const Timestamp &timestamp) {
    timestamp.print(os);
    std::cout << " " << timestamp.read();
    return os;
}
