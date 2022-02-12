#define QUOTE(...) #__VA_ARGS__
static const char* globegenModuleSource = QUOTE(

foreign class ShapeBatching
{
	init() {}

	foreign add(shape, color)

	foreign build_va()
}

);