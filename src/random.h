/**
 * \file random.h
 * \brief A class for generating random numbers
 *
 * Standard library distributions such as std::uniform_int_distribution
 * and std::normal_distribution cannot be used ("https://stackoverflow.com/
 * questions/45936816/non-reproducible-random-numbers-using-random")
 * because the standard does not guarantee repeatability based on the
 * same seed. The following page is very helpful on the subject of
 * generating random numbers: "https://peteroupc.github.io/
 * randomfunc.html#For_Floating_Point_Number_Formats". 
 * 
 * \todo The basic real number distribution in this file should be
 * improved in accordance with the guidance here:
 * "https://peteroupc.github.io/randomfunc.html#Uniform_Random_Real_Numbers"
 * 
 */

#ifndef RANDOM_HPP
#define RANDOM_HPP

#include <random>
#include "seed.h"

template<typename T>
concept Numeric = std::floating_point<T> or std::integral<T>;

/**
 * \brief Random numbers that can be seeded or unseeded
 */
template<Numeric T, typename Gen = std::mt19937_64>
class Random
{
public:

    using seed_t = Gen::result_type; // seed type, std::uint_fast64_t
    
    /**
     * \brief Construct a class for getting random numbers in the
     * range [min,max]
     *
     * Then the class is constructed, it always gets a random seed
     * from /dev/urandom. Then, if necessary, the seed can be set
     * using the seed()
     *
     */
    Random(T min, T max, const Seed<Gen> & seed);
    
    /// Set the seed
    void seed(const Seed<Gen> & seed);

    /// Get the seed
    [[nodiscard]] seed_t seed() const;
    
    /// Get the lower end of the range
    [[nodiscard]] T min() const;

    /// Get the upper end of the range
    [[nodiscard]] T max() const; 

    /// Get a random value in [lower, upper)
    [[nodiscard]] T operator() ();    

private:
    seed_t seed_; 
    Gen gen_;
    T lower_;
    T upper_;
};

/**
 * \brief Generator with static integral limits
 *
 */
template<std::integral T, T Min, T Max, typename Gen = std::mt19937_64>
struct Generator
{
    using result_type = T;
    Random<result_type,Gen> rnd;
    using seed_t = Random<result_type>::seed_t;
    explicit Generator(const Seed<Gen> & seed) : rnd{Min,Max,seed} {}
    result_type operator() () { return rnd(); }
    void seed(seed_t seed) { rnd.seed(seed); }
    [[nodiscard]] seed_t seed() const { return rnd.seed(); }
    constexpr static result_type min() { return Min; }
    constexpr static result_type max() { return Max; }
};

#endif
