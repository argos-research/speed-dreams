#ifndef RANDOM_UTIL_H
#define RANDOM_UTIL_H

#include <random>

class RandomUtil
{
public:
	// Mean is perfectly exact input value and deviation determines how the return value is
	// distributed arround the mean, using normal distribution.
	static double addNormalNoise(double mean, double deviation);

	// Return random number between a and b using uniform distribution
	static double getRandomNumber(double a, double b);

private:
	static std::random_device m_randDevice;
	static std::mt19937 m_generator;
};



#endif