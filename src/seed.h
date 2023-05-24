/**
 * \file seed.h
 * \brief Contains a class for handling the seeds in the program
 *
 * The Seed class is important for enabling the repeatable execution of
 * elements of the program that involve randomness. All instances of 
 * randomness get their randomness from the Random class, which can be
 * seeded to produce the same string of pseudo-random values. The point of
 * the seed class is to store when this class should be seeded
 *
 */

#ifndef SEED_HPP
#define SEED_HPP

#include <iostream>
#include <random>
#include <fstream>

/**
 * \brief Get a random seed
 *
 * According to the page below, it is best to avoid std::random_device,
 * because it is not guaranteed to be random:
 * 
 * https://stackoverflow.com/questions/45069219/
 *   how-to-succinctly-portably-and-thoroughly-
 *   seed-the-mt19937-prng
 *
 * This method uses /dev/urandom to get a random seed.
 * 
 */

/**
 * \brief A class for holding information about seeds used in functions
 *
 */
template<typename Gen = std::mt19937_64>
class Seed
{    
public:
    using seed_t = Gen::result_type;

    /**
     * \brief Initialise to the fixed seed 0
     */
    Seed() : seed_{0} {}

    /**
     * \brief Generate a fixed seed
     */
    explicit constexpr Seed(seed_t seed) : seed_{seed} {}

    /**
     * \brief Set a fixed seed
     */
    void seed(seed_t seed) { seed_ = seed; }

    /**
     * \brief Get the seed
     */
    seed_t seed() const { return seed_; }

    /**
     * \brief Print the seed
     */
    void print(std::ostream & os) const {
	os << "Seed(seed=" << seed_ << ")" << std::endl;
    }

private:
    seed_t seed_{0};
};

#endif
