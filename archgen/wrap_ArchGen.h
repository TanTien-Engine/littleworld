#pragma once

#include <vessel.h>

namespace archgen
{

VesselForeignMethodFn ArchGenBindMethod(const char* signature);
void ArchGenBindClass(const char* class_name, VesselForeignClassMethods* methods);

}