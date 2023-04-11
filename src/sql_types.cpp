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

TimestampOffset operator-(const Timestamp & a, const Timestamp & b) {
    if (a.null() or b.null()) {
	throw std::runtime_error("Cannot subtract null timestamps");
    }
    auto a_time{static_cast<long long>(a.read())};
    auto b_time{static_cast<long long>(b.read())};
    return TimestampOffset{a_time - b_time};
}

TimestampOffset years(long long value) {
    return TimestampOffset{365*24*60*60*value};
}

