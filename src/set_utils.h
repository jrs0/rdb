#ifndef SET_UTILS
#define SET_UTILS

template<typename T>
std::set<T> intersection(const std::set<T> & s1, const std::set<T> & s2) {
    std::set<T> out;
    std::ranges::set_intersection(s1, s2, std::inserter(out, out.begin()));
    return out;
}

template<typename T>
std::set<T> set_union(const std::set<T> & s1, const std::set<T> & s2) {
    std::set<T> out;
    std::ranges::set_union(s1, s2, std::inserter(out, out.begin()));
    return out;
}

// Remove anything in s2 from s1
template<typename T>
std::set<T> set_difference(std::set<T> s1, const std::set<T> & s2) {
    for (const auto & item : s2) {
	s1.erase(item);
    }
    return s1;
}

#endif
