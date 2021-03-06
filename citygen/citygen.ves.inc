#define QUOTE(...) #__VA_ARGS__
static const char* citygenModuleSource = QUOTE(

foreign class Streets
{
	init(tensor_field, border) {}

	foreign build_streamlines(num, random)
	foreign build_topology()

	foreign get_major_paths()
	foreign get_minor_paths()

	foreign get_nodes()
	foreign get_polygons()

	foreign set_seed(seed)
	foreign set_ortho(ortho)	

	foreign set_nodes(pts)
}

foreign class ParcelsOBB
{
	init(polygon) {}

	foreign build(max_len)

	foreign get_polygons()

	foreign set_seed(seed)
	foreign set_density_center(center)
}

foreign class ParcelsSS
{
	init(polygon) {}

	foreign build(max_len)

	foreign get_polygons()

	foreign set_seed(seed)
	foreign set_density_center(center)
}

foreign class Heightfield
{
	init(texture) {}

	foreign get_height(x, y)
}

foreign class GeometryTools
{
	foreign static polyline_offset(polyline, distance, is_closed)
	foreign static polyline_expand(polyline, offset)

	foreign static shape_l(polygon, front_width, left_width, remainder)
	foreign static shape_u(polygon, front_width, left_width, right_width, remainder)
	foreign static shape_o(polygon, front_width, back_width, left_width, right_width, remainder)

	foreign static is_counterclockwise(polygon)

	foreign static polygon_extrude_face(polygon, distance)
	foreign static polygon_extrude_skeleton(polygon, distance)

	foreign static polytope_add_height(polytope, heightfield)
}

);