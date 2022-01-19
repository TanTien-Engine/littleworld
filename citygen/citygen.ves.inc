#define QUOTE(...) #__VA_ARGS__
static const char* citygenModuleSource = QUOTE(

foreign class Streets
{
	init(tensor_field, border) {}

	foreign build_streamlines(num)
	foreign build_topology()

	foreign get_major_paths()
	foreign get_minor_paths()

	foreign get_nodes()
	foreign get_polygons()

	foreign set_seed(seed)
}

foreign class Block
{
	init(tris) {}

	foreign offset(distance)

	foreign get_border()
}

foreign class ParcelsOBB
{
	init(polygon) {}

	foreign build(max_len)

	foreign offset(distance)

	foreign get_polygons()

	foreign set_seed(seed)
}

foreign class ParcelsSS
{
	init(polygon) {}

	foreign build()

	foreign get_polygons(dist)
}

foreign class GeometryTools
{
	foreign static polyline_expand(polyline, offset)
}

);