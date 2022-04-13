#define QUOTE(...) #__VA_ARGS__
static const char* archgenModuleSource = QUOTE(

foreign class TextureMapping
{
	init(axes_x, axes_y, scale, offset, uw_factor) {}

	foreign clone()
}

foreign class MeshBuilder
{
	foreign static build_mesh_from_poly(polys, tex_mapping)
	foreign static build_mesh_from_file(filepath, tex_mapping)
}

foreign class ScopeTools
{
	foreign static face_mapping(poly, face)
	foreign static calc_insert_mat(aabb, scope)
	foreign static get_scope_size(scope)

	foreign static comp_edges(poly)
	foreign static comp_face_edges(poly)
}

foreign class RoofExtrude
{
	foreign static hip(poly, distance)
	foreign static pyramid(poly, distance)
	foreign static shed(poly, distance, index)
	foreign static gable(poly, distance)
}

);