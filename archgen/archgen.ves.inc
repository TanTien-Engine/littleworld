#define QUOTE(...) #__VA_ARGS__
static const char* archgenModuleSource = QUOTE(

foreign class TextureMapping
{
	init(axes_x, axes_y, scale, offset) {}

	foreign clone()
}

foreign class MeshBuilder
{
	foreign static build_mesh(polys, tex_mapping)
	foreign static build_mesh(filepath)
}

foreign class ArchTools
{
	foreign static calc_geo_mat(aabb, scope)
}

);