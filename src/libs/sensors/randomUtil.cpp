#include "randomUtil.h"

std::random_device RandomUtil::m_randDevice;
std::mt19937 RandomUtil::m_generator = std::mt19937(RandomUtil::m_randDevice());

double RandomUtil::addNormalNoise(double mean, double deviation)
{
	std::normal_distribution<> distribution(mean, deviation);
	return distribution(m_generator);
}

double RandomUtil::getRandomNumber(double a, double b)
{
	std::uniform_real_distribution<> distribution(a, b);
	return distribution(m_generator);
}