#ifndef EVENT_COUNTER_HPP
#define EVENT_COUNTER_HPP

#include "clinical_code.hpp"

class EventCounter {
public:
    
    /// Increment a group counter in the before map
    void push_before(const ClinicalCodeGroup & group) {
	before_counts_[group]++;
    }

    void push_after(const ClinicalCodeGroup & group) {
	after_counts_[group]++;
    }

    const auto & counts_before() const {
	return before_counts_;
    }

    const auto & counts_after() const {
	return after_counts_;
    }

    void print(std::shared_ptr<StringLookup> lookup) const {
	std::cout << "- Counts before:" << std::endl;
	for (const auto & [group, count] : before_counts_) {
	    std::cout << "  - ";
	    group.print(lookup);
	    std::cout << ": " << count
		      << std::endl;
	}
	std::cout << "- Counts after:" << std::endl;
	for (const auto & [group, count] : after_counts_) {
	    std::cout << "  - ";
	    group.print(lookup);
	    std::cout << ": " << count
		      << std::endl;
	}
    }

private:
    std::map<ClinicalCodeGroup, std::size_t> before_counts_;
    std::map<ClinicalCodeGroup, std::size_t> after_counts_;
};


#endif
