#pragma once

#include <vessel.h>

namespace globegen
{

VesselForeignMethodFn GlobeGenBindMethod(const char* signature);
void GlobeGenBindClass(const char* class_name, VesselForeignClassMethods* methods);

}