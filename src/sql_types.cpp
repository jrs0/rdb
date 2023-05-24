#include "sql_types.h"

std::ostream &operator<<(std::ostream &os, const Integer &integer) {
    integer.print(os);
    return os;
}

std::ostream &operator<<(std::ostream &os, const Timestamp &timestamp) {
    timestamp.print(os);
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

std::ostream & operator << (std::ostream & os, const TimestampOffset & offset) {
    auto value{offset.value()};
    if (value > 0) {
	os << "+ ";
    } else {
	os << "- ";
    }
    auto years{std::abs(value) / (365*24*60*60)};
    auto less_than_year{value % (365*24*60*60)};
    auto days{less_than_year / (24*60*60)};
    auto less_than_day{less_than_year % (24*60*60)};
    auto hours{less_than_day / (60*60)};
    auto less_than_hour{less_than_day % (60*60)};
    auto minutes{less_than_hour / (60)};
    auto seconds{less_than_hour % (60)};

    if (years > 0) {
	os << years << "y ";
    }

    if (days > 0) {
	os << days << "d ";
    }

    if (hours > 0) {
	os << hours << "h ";
    }

    if (minutes > 0) {
	os << minutes << "m ";
    }

    os << seconds << "s (" << value << ")";
    
    return os;
}
