#pragma once

#include <vector>
#include <time.h>

namespace citygen
{

class Random
{
public:
	template<class T>
		static void RandomPermutation(std::vector<T>& src);

}; // Random

template<class T>
void Random::RandomPermutation(std::vector<T>& src)
{
//	srand((unsigned)time(NULL));
	size_t range = src.size() - 1;

	for(size_t i = src.size() - 1; i > 1; --i) {
		size_t rndIndex = int(__int64(range) * rand() / (RAND_MAX + 1.0 ));
		std::swap(src.at(i), src.at(rndIndex));
	}
}

}