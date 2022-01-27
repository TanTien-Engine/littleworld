#pragma once

#include "SM_Vector.h"

#include <vector>

namespace citygen
{

std::vector<sm::vec2> smooth_chaikin(const std::vector<sm::vec2>& polyline, size_t times);

}