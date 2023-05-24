/**
 * \file random.cpp
 * \brief Implementation of the Random class
 *
 */

#include "random.h"

template<Numeric T, typename Gen>
Random<T, Gen>::Random(T lower, T upper, const Seed<Gen> & seed)
    : seed_{seed.seed()}, gen_{seed_}, lower_{lower}, upper_{upper}
{
    if (lower > upper) {
	throw std::logic_error("Cannot create Random class when "
			       "lower > upper");
    }
}

template<Numeric T, typename Gen>
void Random<T, Gen>::seed(const Seed<Gen> & seed)
{
    seed_ = seed.seed(); // Store the seed
    gen_.seed(seed_); // Set the new seed
}

template<Numeric T, typename Gen>
Random<T, Gen>::seed_t Random<T, Gen>::seed() const
{
    return seed_;  
}


template<Numeric T, typename Gen>
T Random<T, Gen>::min() const
{
    return lower_;
}

template<Numeric T, typename Gen>
T Random<T, Gen>::max() const 
{
    return upper_;
}

/// Get a random value in [lower, upper)
template<Numeric T, typename Gen>
T Random<T, Gen>::operator() ()
{
    seed_t val{gen_()};
    if constexpr (std::is_integral_v<T>) {
	auto range{upper_ + 1 - lower_};
	return lower_ + (val % range);
    } else {
	// Range of gen_ integers
	constexpr auto range{Gen::max() - Gen::min()};
	// Generate float
	return lower_ + gen_()/static_cast<T>(range/(upper_-lower_));
    }
}

// Instantiations
template class Random<int, std::mt19937_64>;
template class Random<long int, std::mt19937_64>;
template class Random<std::size_t, std::mt19937_64>;
template class Random<unsigned, std::mt19937_64>;

template class Random<float, std::mt19937_64>;
template class Random<double, std::mt19937_64>;
template class Random<long double, std::mt19937_64>;

template class Random<int, std::mt19937>;
template class Random<long int, std::mt19937>;
template class Random<std::size_t, std::mt19937>;
template class Random<unsigned, std::mt19937>;

template class Random<float, std::mt19937>;
template class Random<double, std::mt19937>;
template class Random<long double, std::mt19937>;
