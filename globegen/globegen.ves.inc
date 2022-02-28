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
	init(filepath) {}

	foreign get_feedback_tex()
	foreign get_atlas_tex()
	foreign get_page_table_tex()

	// [ vtex_w, vtex_h, tile_sz, border_sz ]
	foreign get_size()

	foreign update(mvp_mat)
}

foreign class GlobeTools
{
	foreign static build_vtex(src, dst, vtex_w, vtex_h, tile_sz, border_sz)
	foreign static build_vtex2(src0, src1, dst, vtex_w, vtex_h, tile_sz, border_sz)

	foreign static prepare_dem15(src, dst)
	foreign static build_vtex_tiles(src, tile_num_x, tile_num_y, dst, vtex_w, vtex_h, tile_sz, border_sz)

	foreign static split_image(src, num_x, num_y)
}

);