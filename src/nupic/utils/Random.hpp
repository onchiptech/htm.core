/* ---------------------------------------------------------------------
 * Numenta Platform for Intelligent Computing (NuPIC)
 * Copyright (C) 2013, Numenta, Inc.  Unless you have an agreement
 * with Numenta, Inc., for a separate license for this software code, the
 * following terms and conditions apply:
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero Public License for more details.
 *
 * You should have received a copy of the GNU Affero Public License
 * along with this program.  If not, see http://www.gnu.org/licenses.
 *
 * http://numenta.org/licenses/
 * ---------------------------------------------------------------------
 */

/** @file
    Random Number Generator interface
*/

#ifndef NTA_RANDOM_HPP
#define NTA_RANDOM_HPP

#include <algorithm>
#include <iterator>
#include <random>
#include <string>
#include <vector>

#include <nupic/types/Types.hpp>
#include <nupic/types/Serializable.hpp>
#include <nupic/utils/Log.hpp>

namespace nupic {

/**
 * @b Responsibility
 * Provides standardized random number generation for the NuPIC Runtime Engine.
 * Seed can be logged in one run and then set in another.
 * @b Rationale
 * Makes it possible to reproduce tests that are driven by random number
 * generation.
 * This class produces same Random sequence (for given seed) on all Platforms
 * (compiler, stdlib, OS), it can be fully deterministic. 
 *
 * @b Description
 * Functionality is similar to the standard random function that is provided
 * by C++ Uniform Random Distribution.
 *
 * Each Random object is a random number generator. There are three ways of
 * creating one:
 * 1) explicit seed
 *       Random rng(seed);
 * 2) self-seeded
 *       Random rng;
 *
 * Good self-seeds are generated by an internal global random number generator.
 *
 * Automated tests that use random numbers should normally use named generators.
 * This allows them to get a different seed each time, but also allows
 * reproducibility in the case that a test failure is triggered by a particular
 * seed.
 *
 * @todo Add ability to specify different rng algorithms.
 *
 * API:
 * there are 2 main functions,
 * getReal()
 * getUInt()
 *
 */
class Random : public Serializable  {
public:
  Random(UInt64 seed = 0/*=random seed*/);

  // save and load serialized data
  void save(std::ostream &stream) const override { stream << *this; }
  void load(std::istream &stream) override { stream >> *this; }
  void saveToFile(std::string filePath) const override { Serializable::saveToFile(filePath); }
  void loadFromFile(std::string filePath) override { Serializable::loadFromFile(filePath); }


  bool operator==(const Random &other) const;
  inline bool operator!=(const Random &other) const {
    return !operator==(other);
  }

  //main API methods:
  /** return a value (uniformly) distributed between [0,max)
   */
  inline UInt32 getUInt32(const UInt32 max = MAX32) {
    NTA_ASSERT(max > 0);
    return gen() % max; //uniform_int_distribution(gen) replaced, as is not same on all platforms! 
  }

  /** return a double uniformly distributed on [0,1.0)
   * May not be cross-platform (but currently is to our experience)
   */
  inline double getReal64() {
    return gen() / (Real64) max();
  }

  // populate choices with a random selection of nChoices elements from
  // population. throws exception when nPopulation < nChoices
  // templated functions must be defined in header
  //TODO replace with std::sample in c++17 : https://en.cppreference.com/w/cpp/algorithm/sample 
  template <class T>
  std::vector<T> sample(const std::vector<T>& population, UInt nChoices) {
    if (nChoices == 0) {
      return std::vector<T>(0);
    }
    NTA_CHECK(nChoices <= population.size()) << "population size must be greater than number of choices";
    std::vector<T> pop(population); //deep copy
    this->shuffle(std::begin(pop), std::end(pop));
    pop.resize(nChoices); //keep only first nChoices, drop rest
    return pop;
  }
  //compatibility method for Py-bindings, //TODO remove with SWIG 
  template<class T>
  void sample(const T population[], UInt nPopulation, T choices[], UInt nChoices) {
    std::vector<T> vPop(population, population + nPopulation); 
    std::vector<T> vChoices = this->sample<T>(vPop, nChoices);
    std::copy(vChoices.begin(), vChoices.end(), choices);
  }

  // randomly shuffle the elements
  template <class RandomAccessIterator>
  void shuffle(RandomAccessIterator first, RandomAccessIterator last) {
    std::shuffle(first, last, gen);
  }

  // for STL compatibility
  UInt32 operator()(UInt32 n = MAX32) { 
	  NTA_ASSERT(n > 0);
	  return getUInt32(n); 
  }

  // normally used for debugging only
  UInt64 getSeed() const { return seed_; }

  // for STL
  typedef unsigned long argument_type;
  typedef unsigned long result_type;
  result_type max() const { return gen.max(); }
  result_type min() const { return gen.min(); }
  static const UInt32 MAX32 = (UInt32)((Int32)(-1));

protected:
  friend class RandomTest;
  friend std::ostream &operator<<(std::ostream &, const Random &);
  friend std::istream &operator>>(std::istream &, Random &);
  friend UInt32 GetRandomSeed();
private:
  UInt64 seed_;
  std::mt19937_64 gen; //Standard mersenne_twister_engine 64bit seeded with seed_
//  std::random_device rd; //HW random for random seed cases, undeterministic -> problems with op= and copy-constructor, therefore disabled


};

// serialization/deserialization
std::ostream &operator<<(std::ostream &, const Random &);
std::istream &operator>>(std::istream &, Random &);

// This function returns seeds from the Random singleton in our
// "universe" (application, plugin, python module). If, when the
// Random constructor is called, seeder_ is NULL, then seeder_ is
// set to this function. The plugin framework can override this
// behavior by explicitly setting the seeder to the RandomSeeder
// function provided by the application.
UInt32 GetRandomSeed();

} // namespace nupic

#endif // NTA_RANDOM_HPP
