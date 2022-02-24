#define QUOTE(...) #__VA_ARGS__
static const char* globegenModuleSource = QUOTE(

foreign class ShapeBatching
{
	init() {}

	foreign add(shape, color)

	foreign build_va()
}

foreign class VirtualTexture
{
	init(filepath, vtex_sz, tile_sz, border_sz) {}

	foreign get_feedback_tex()
	foreign get_atlas_tex()
	foreign get_page_table_tex()

	foreign update(vp_mat, heightmap)
}

foreign class GlobeTools
{
	foreign static build_vtex(src, dst, vtex_sz, tile_sz, border_sz)
}

);