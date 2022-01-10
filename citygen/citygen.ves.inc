#define QUOTE(...) #__VA_ARGS__
static const char* citygenModuleSource = QUOTE(

foreign class Network
{
	init(tensor_field) {}

	foreign build_streamlines(num)
	foreign build_topology()

	foreign get_major_paths()
	foreign get_minor_paths()

	foreign get_nodes()
	foreign get_polygons()
}

);