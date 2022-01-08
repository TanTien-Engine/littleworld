#pragma once

#include <vessel.h>

namespace citygen
{

VesselForeignMethodFn CityGenBindMethod(const char* signature);
void CityGenBindClass(const char* class_name, VesselForeignClassMethods* methods);

}