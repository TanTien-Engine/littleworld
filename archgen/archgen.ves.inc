#define QUOTE(...) #__VA_ARGS__
static const char* archgenModuleSource = QUOTE(

foreign class TextureMapping
{
	init(axes_x, axes_y, scale, offset) {}

	foreign clone()
}

foreign class GeometryTools
{
	foreign static build_mesh(polys, tex_mapping)
	foreign static build_mesh(filepath)
}

);