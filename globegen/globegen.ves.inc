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
	init(filepath, sphere) {}

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
	foreign static merge_dem15(src, dst)
	foreign static build_vtex_tiles(src, tile_num_x, tile_num_y, dst, vtex_w, vtex_h, tile_sz, border_sz)

	foreign static split_image(src, num_x, num_y)

	// globe camera
	foreign static cam_view_mat(lon, lat, theta, phi, d)
	foreign static cam_proj_mat(lon, lat, theta, phi, d)
	foreign static cam_position(lon, lat, theta, phi, d)
	// horizon camera
	foreign static hori_cam_view_mat(h, theta, phi)
	foreign static hori_cam_proj_mat(near, far)

	// ocean
	foreign static gen_ocean_mesh(grid_size, cam_theta)

	// fft ocean
	foreign static generate_bit_reversed_indices(n)
}

);