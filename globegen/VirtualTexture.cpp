#include "VirtualTexture.h"

#include "modules/model/Model.h"
#include "modules/render/Render.h"

#include <unirender/Device.h>
#include <unirender/Context.h>
#include <unirender/Texture.h>
#include <unirender/TextureDescription.h>
#include <unirender/Framebuffer.h>
#include <unirender/ShaderProgram.h>
#include <unirender/Uniform.h>
#include <unirender/ClearState.h>
#include <unirender/DrawState.h>
#include <shadertrans/ShaderTrans.h>

#include <algorithm>

namespace
{

const int UPLOADS_PER_FRAME = 6;
const int ATLAS_SIZE = 2064;

const int FEEDBACK_SIZE = 64;
const int MIP_SAMPLE_BIAS = 3;

const char* feedback_plane_vs = R"(

#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texcoord;

uniform UBO
{
	mat4 mvp_mat;
};

out VS_OUT {
    vec2 texcoord;
} vs_out;

void main()
{
	vs_out.texcoord = vec2(texcoord.x, 1.0 - texcoord.y);
	gl_Position = mvp_mat * vec4(position.xzy, 1.0);
}

)";

const char* feedback_sphere_vs = R"(

#version 330 core
layout (location = 0) in vec3 position;

uniform UBO
{
	mat4 mvp_mat;
};

out VS_OUT {
    vec2 texcoord;
} vs_out;

vec3 GeodeticSurfaceNormal(vec3 positionOnEllipsoid, vec3 oneOverEllipsoidRadiiSquared)
{
    return normalize(positionOnEllipsoid * oneOverEllipsoidRadiiSquared);
}

vec2 ComputeTextureCoordinates(vec3 normal)
{
    return vec2(atan(normal.x, normal.z) / (2 * 3.14159) + 0.5, asin(normal.y) / 3.14159 + 0.5);
}

void main()
{
	vec3 normal = GeodeticSurfaceNormal(position, vec3(1));
	vec2 tex_coord = ComputeTextureCoordinates(normal);
	
	vs_out.texcoord = tex_coord;
	gl_Position = mvp_mat * vec4(position, 1.0);
}

)";

const char* feedback_fs = R"(

#version 330 core

uniform UBO
{
	vec2 u_page_table_size;
	vec2 u_virt_tex_size;
	float u_mip_sample_bias;
};

in VS_OUT {
    vec2 texcoord;
} fs_in;

float tex_mip_level(vec2 coord, vec2 tex_size)
{
   vec2 dx_scaled, dy_scaled;
   vec2 coord_scaled = coord * tex_size;

   dx_scaled = dFdx(coord_scaled);
   dy_scaled = dFdy(coord_scaled);

   vec2 dtex = dx_scaled*dx_scaled + dy_scaled*dy_scaled;
   float min_delta = max(dtex.x,dtex.y);
   float miplevel = max(0.5 * log2(min_delta), 0.0);

   return miplevel;
}

void main()
{
	float mip = floor(tex_mip_level(fs_in.texcoord, u_virt_tex_size) - u_mip_sample_bias);
	mip = clamp(mip, 0.0, log2(min(u_page_table_size.x, u_page_table_size.y)));

	vec2 times = u_page_table_size / exp2(mip);
	vec2 offset = floor(fs_in.texcoord * times);
	offset = clamp(offset, vec2(0.0, 0.0), times - vec2(1.0));

	gl_FragColor = vec4(vec3(offset / 255.0, mip / 255.0), 1.0);
}

)";

}

namespace globegen
{

VirtualTexture::VirtualTexture(const char* filepath, VTexGeoType geo_type)
	: m_geo_type(geo_type)
{
	m_file.open(filepath, std::ios::in | std::ios::binary);

	m_file.seekg(0, std::ios_base::beg);
	m_file.read(reinterpret_cast<char*>(&m_info), sizeof(m_info));

	m_indexer = std::make_shared<PageIndexer>(m_info);
	m_feedback_buf = std::make_shared<FeedbackBuffer>(*m_indexer, m_info, geo_type);
	m_tiles_file = std::make_shared<TileDataFile>(m_info, m_file);
	m_atlas = std::make_shared<TextureAtlas>(ATLAS_SIZE, m_info.tile_size, m_info.border_size);
	m_table = std::make_shared<PageTable>(m_info.vtex_width / m_info.tile_size, m_info.vtex_height / m_info.tile_size);
	m_cache = std::make_shared<PageCache>(*m_atlas, *m_table, *m_indexer, *m_tiles_file);
}

void VirtualTexture::Update(const sm::mat4& mvp_mat, const sm::vec2& screen_sz)
{
	auto requests = m_feedback_buf->Update(mvp_mat, screen_sz);

	m_toload.clear();
	int touched = 0;
	for (size_t i = 0, n = requests.size(); i < n; ++i)
	{
		if (requests[i] == 0) {
			continue;
		}

		auto& page = m_indexer->QueryPageByIdx(i);
		if (!m_cache->Touch(page)) {
			m_toload.push_back(PageWithCount(page, requests[i]));
		} else {
			++touched;
		}
	}

	size_t page_n = m_atlas->GetPageCount();
	if (touched < page_n * page_n)
	{
		std::sort(m_toload.begin(), m_toload.end(), [](const PageWithCount& p0, const PageWithCount& p1)->bool {
			if (p0.page.mip != p1.page.mip) {
				return p0.page.mip < p1.page.mip;
			} else {
				return p0.count > p1.count;
			}
		});

		int load_n = std::min((int)m_toload.size(), UPLOADS_PER_FRAME);
		for (int i = 0; i < load_n; ++i) {
			m_cache->Request(m_toload[i].page);
		}
	}
	else
	{
		m_feedback_buf->DecreaseMipBias();
	}

	m_table->Update();
}

//std::shared_ptr<ur::Texture> VirtualTexture::LoadToTexture() const
//{
//	const int LOD_LEVEL = 0;
//
//	auto dev = tt::Render::Instance()->Device();
//
//	const int tex_sz = m_vtex_sz >> LOD_LEVEL;
//
//	ur::TextureDescription desc;
//	desc.width = tex_sz;
//	desc.height = tex_sz;
//	desc.format = ur::TextureFormat::R16F;
//	auto tex = dev->CreateTexture(desc);
//
//	size_t data_sz = m_tile_sz * m_tile_sz * 4;
//	uint8_t* data = new uint8_t[data_sz];
//
//	int offset = 0;
//	for (int i = 0; i < LOD_LEVEL; ++i) {
//		offset += static_cast<int>(std::pow((m_vtex_sz / m_tile_sz) >> i, 2));
//	}
//
//	const int tile_num = (m_vtex_sz / m_tile_sz) >> LOD_LEVEL;
//	const size_t tile_sz = m_tile_sz << LOD_LEVEL;
//	for (int y = 0; y < tile_num; ++y) {
//		for (int x = 0; x < tile_num; ++x) {
//			m_file.seekg(static_cast<uint64_t>(data_sz) * (offset + y * tile_num + x));
//			m_file.read(reinterpret_cast<char*>(data), data_sz);
//			tex->Upload(data, x * m_tile_sz, y * m_tile_sz, m_tile_sz, m_tile_sz);
//		}
//	}
//
//	delete[] data;
//
//	return tex;
//}

//////////////////////////////////////////////////////////////////////////
// struct VirtualTexture::Page
//////////////////////////////////////////////////////////////////////////

VirtualTexture::Page::Page(int x, int y, int mip)
	: x(x), y(y), mip(mip)
{
}

bool VirtualTexture::Page::operator == (const Page& p) const
{
	return x == p.x && y == p.y && mip == p.mip;
}

//////////////////////////////////////////////////////////////////////////
// class VirtualTexture::PageIndexer
//////////////////////////////////////////////////////////////////////////

VirtualTexture::PageIndexer::
PageIndexer(const VTexInfo& info)
{
	m_mip_count = static_cast<int>(std::log2(std::min(info.vtex_width, info.vtex_height) / info.tile_size)) + 1;

	m_sizes.resize(m_mip_count);
	for (int i = 0; i < m_mip_count; ++i)
	{
		m_sizes[i].x = static_cast<int>(std::ceil((info.vtex_width  >> i) / static_cast<float>(info.tile_size)));
		m_sizes[i].y = static_cast<int>(std::ceil((info.vtex_height >> i) / static_cast<float>(info.tile_size)));
	}

	m_offsets.resize(m_mip_count);
	m_page_count = 0;
	for (int i = 0; i < m_mip_count; ++i) {
		m_offsets[i] = m_page_count;
		m_page_count += m_sizes[i].x * m_sizes[i].y;
	}

	m_pages.resize(m_page_count);
	for (int i = 0; i < m_mip_count; ++i)
	{
		for (int y = 0; y < m_sizes[i].y; ++y) {
			for (int x = 0; x < m_sizes[i].x; ++x) {
				Page p(x, y, i);
				m_pages[CalcPageIdx(p)] = p;
			}
		}
	}
}

int VirtualTexture::PageIndexer::
CalcPageIdx(const Page& page) const
{
	assert(page.mip >= 0 && page.mip < m_mip_count);
	int offset = m_offsets[page.mip];
	int stride = m_sizes[page.mip].x;
	return offset + page.y * stride + page.x;
}

const VirtualTexture::Page& VirtualTexture::PageIndexer::
QueryPageByIdx(size_t idx) const
{
	assert(idx < static_cast<int>(m_pages.size()));
	return m_pages[idx];
}

//////////////////////////////////////////////////////////////////////////
// class VirtualTexture::FeedbackBuffer
//////////////////////////////////////////////////////////////////////////

VirtualTexture::FeedbackBuffer::
FeedbackBuffer(const PageIndexer& page_idx, const VTexInfo& info, VTexGeoType geo_type)
	: m_indexer(page_idx)
	, m_mip_bias(MIP_SAMPLE_BIAS)
{
	auto dev = tt::Render::Instance()->Device();

	const char* feedback_vs = nullptr;
	switch (geo_type)
	{
	case VTexGeoType::Plane:		
		m_feedback_vao = tt::Model::Instance()->CreateGrids(*dev, ur::VertexLayoutType::PosTex, FEEDBACK_SIZE);
		feedback_vs = feedback_plane_vs;
		break;
	case VTexGeoType::Spherre:
	{
		ur::PrimitiveType prim_type;
		m_feedback_vao = tt::Model::Instance()->CreateShape(*dev, tt::ShapeType::Sphere, ur::VertexLayoutType::PosTex, prim_type);
		feedback_vs = feedback_sphere_vs;
	}
		break;
	default:
		assert(0);
	}

	m_page_table_size = sm::ivec2(info.vtex_width, info.vtex_height) / info.tile_size;

	ur::TextureDescription desc;
	desc.width = FEEDBACK_SIZE;
	desc.height = FEEDBACK_SIZE;
	desc.format = ur::TextureFormat::RGBA8;
	m_tex = dev->CreateTexture(desc);

	m_fbo = dev->CreateFramebuffer();
	m_fbo->SetAttachment(ur::AttachmentType::Color0, ur::TextureTarget::Texture2D, m_tex, nullptr);

	std::vector<unsigned int> vs, fs;
	shadertrans::ShaderTrans::GLSL2SpirV(shadertrans::ShaderStage::VertexShader, feedback_vs, nullptr, vs);
	shadertrans::ShaderTrans::GLSL2SpirV(shadertrans::ShaderStage::PixelShader, feedback_fs, nullptr, fs);

	m_shader = dev->CreateShaderProgram(vs, fs);
	m_buf = new uint8_t[FEEDBACK_SIZE * FEEDBACK_SIZE * 4];
	memset(m_buf, 0, FEEDBACK_SIZE * FEEDBACK_SIZE * 4);

	auto u_page_table_size = m_shader->QueryUniform("u_page_table_size");
	assert(u_page_table_size);
	u_page_table_size->SetValue(sm::vec2(info.vtex_width / info.tile_size, info.vtex_height / info.tile_size).xy, 2);

	auto u_virt_tex_size = m_shader->QueryUniform("u_virt_tex_size");
	assert(u_virt_tex_size);
	u_virt_tex_size->SetValue(sm::vec2(info.vtex_width, info.vtex_height).xy, 2);

	auto u_mip_sample_bias = m_shader->QueryUniform("u_mip_sample_bias");
	assert(u_mip_sample_bias);
	float bias = static_cast<float>(m_mip_bias);
	u_mip_sample_bias->SetValue(&m_mip_bias, 1);

	m_requests.resize(m_indexer.GetPageCount(), 0);
}

VirtualTexture::FeedbackBuffer::
~FeedbackBuffer()
{
	delete[] m_buf;
}

std::vector<int> VirtualTexture::FeedbackBuffer::Update(const sm::mat4& mvp_mat, const sm::vec2& screen_sz)
{
	// prepare
	auto u_mvp_mat = m_shader->QueryUniform("mvp_mat");
	assert(u_mvp_mat);
	u_mvp_mat->SetValue(mvp_mat.x, 16);

	return Draw(screen_sz);
}

void VirtualTexture::FeedbackBuffer::DecreaseMipBias()
{
	--m_mip_bias;
	if (m_mip_bias < 0) {
		m_mip_bias = 0;
	}

	auto u_mip_sample_bias = m_shader->QueryUniform("u_mip_sample_bias");
	assert(u_mip_sample_bias);
	float bias = static_cast<float>(m_mip_bias);
	u_mip_sample_bias->SetValue(&m_mip_bias, 1);
}

std::vector<int> VirtualTexture::FeedbackBuffer::
Draw(const sm::vec2& screen_sz)
{
	auto ctx = tt::Render::Instance()->Context();

	int x, y, w, h;
	ctx->GetViewport(x, y, w, h);
	ctx->SetViewport(0, 0, m_tex->GetWidth(), m_tex->GetHeight());

	ctx->SetFramebuffer(m_fbo);

	ur::ClearState cs;
	cs.color.FromRGBA(0);
	cs.buffers = ur::ClearBuffers::ColorAndDepthBuffer;

	ctx->Clear(cs);

	ur::DrawState ds;

	ds.render_state.depth_test.enabled = true;
	ds.render_state.depth_test.function = ur::DepthTestFunc::LessThanOrEqual;

	ds.program = m_shader;
	ds.vertex_array = m_feedback_vao;

	ctx->Draw(ur::PrimitiveType::Triangles, ds, nullptr);

	Download();
	auto ret = m_requests;
	std::fill(m_requests.begin(), m_requests.end(), 0);

	ctx->SetFramebuffer(nullptr);
	ctx->SetViewport(x, y, w, h);

	return ret;
}

void VirtualTexture::FeedbackBuffer::
Download()
{
	auto dev = tt::Render::Instance()->Device();
	dev->ReadPixels(m_buf, ur::TextureFormat::RGBA8, 0, 0, FEEDBACK_SIZE, FEEDBACK_SIZE);

    int page_table_size_log2 = static_cast<int>(std::log2(std::min(m_page_table_size.x, m_page_table_size.y)));
	for (int i = 0, n = FEEDBACK_SIZE * FEEDBACK_SIZE; i < n; ++i)
	{
		if (m_buf[i * 4 + 3] == 255)
		{
			int x = static_cast<int>(m_buf[i * 4]);
			int y = static_cast<int>(m_buf[i * 4 + 1]);
			int mip = static_cast<int>(m_buf[i * 4 + 2]);

			int count = page_table_size_log2 - mip + 1;
			for (int j = 0; j < count; ++j)
			{
				int _x = x >> j;
				int _y = y >> j;
				int _mip = mip + j;
				int idx = m_indexer.CalcPageIdx(Page(_x, _y, _mip));
				++m_requests[idx];
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// class VirtualTexture::TextureAtlas
//////////////////////////////////////////////////////////////////////////

VirtualTexture::TextureAtlas::
TextureAtlas(size_t atlas_sz, size_t page_sz, size_t border_sz)
	: m_page_sz(page_sz)
	, m_border_sz(border_sz)
	, m_page_count(atlas_sz / (page_sz + border_sz * 2))
{
	auto dev = tt::Render::Instance()->Device();

	uint8_t* pixels = new uint8_t[atlas_sz * atlas_sz * 2];
	memset(pixels, 0, atlas_sz * atlas_sz * 2);

	ur::TextureDescription desc;
	desc.width = atlas_sz;
	desc.height = atlas_sz;
	desc.format = ur::TextureFormat::R16F;
	m_tex = dev->CreateTexture(desc, pixels);

	delete[] pixels;
}

VirtualTexture::TextureAtlas::
~TextureAtlas()
{
}

void VirtualTexture::TextureAtlas::
UploadPage(const uint8_t* pixels, int x, int y)
{
	const size_t size = m_page_sz + m_border_sz * 2;
	m_tex->Upload(pixels, x * size, y * size, size, size);
}

//////////////////////////////////////////////////////////////////////////
// class VirtualTexture::PageTable
//////////////////////////////////////////////////////////////////////////

VirtualTexture::PageTable::
PageTable(size_t width, size_t height)
	: m_width(width)
	, m_height(height)
{
	const size_t level = CalcMaxLevel();
	m_root = std::make_unique<QuadNode>(level, Rect(0, 0, m_width, m_height));

	m_data.resize(level + 1);
	for (size_t i = 0; i < level + 1; ++i)
	{
		int sw = width >> i;
		int sh = height >> i;

		auto& data = m_data[i];
		data.w = sw;
		data.h = sh;
		data.data = new uint8_t[sw * sh * 4];
		memset(data.data, 0, sw * sh * 4);
	}

	ur::TextureDescription desc;
	desc.width = m_width;
	desc.height = m_height;
	desc.format = ur::TextureFormat::RGBA8;
	desc.gen_mipmaps = true;
	m_tex = tt::Render::Instance()->Device()->CreateTexture(desc, nullptr);
}

void VirtualTexture::PageTable::
AddPage(const Page& page, int mapping_x, int mapping_y)
{
	int scale = 1 << page.mip;
	int x = page.x * scale;
	int y = page.y * scale;

	QuadNode* node = m_root.get();
	while (page.mip < node->level)
	{
		for (int i = 0; i < 4; ++i)
		{
			Rect cr = node->CalcChildRect(i);
			if (!cr.Contain(x, y)) {
				continue;
			}
			if (node->children[i] == nullptr) {
				node->children[i] = std::make_unique<QuadNode>(node->level - 1, cr);
			}
			node = node->children[i].get();
			break;
		}
	}

	node->mapping_x = mapping_x;
	node->mapping_y = mapping_y;
}

void VirtualTexture::PageTable::
RemovePage(const Page& page)
{
	int index;
	auto node = FindPage(page, index);
	if (node != nullptr) {
		node->children[index] = nullptr;
	}
}

void VirtualTexture::PageTable::
Update()
{
	const auto level = CalcMaxLevel();
	for (size_t i = 0; i < level + 1; ++i)
	{
		m_root->Write(m_data[i].w, m_data[i].h, m_data[i].data, i);
		m_tex->Upload(m_data[i].data, 0, 0, m_data[i].w, m_data[i].h, i);
	}
}

size_t VirtualTexture::PageTable::
CalcMaxLevel() const
{
	return static_cast<size_t>(std::min(std::log2(m_width), std::log2(m_height)));
}

VirtualTexture::PageTable::QuadNode* VirtualTexture::PageTable::
FindPage(const VirtualTexture::Page& page, int& index) const
{
	QuadNode* node = m_root.get();

	int scale = 1 << page.mip;
	int x = page.x * scale;
	int y = page.y * scale;

	bool exitloop = false;
	while (!exitloop)
	{
		exitloop = true;
		for (int i = 0; i < 4; ++i)
		{
			if (node->children[i] != nullptr && node->children[i]->rect.Contain(x, y))
			{
				if (page.mip == node->level - 1)
				{
					index = i;
					return node;
				}
				else
				{
					node = node->children[i].get();
					exitloop = false;
				}
			}
		}
	}

	index = -1;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
// class VirtualTexture::PageTable::QuadNode
//////////////////////////////////////////////////////////////////////////

VirtualTexture::PageTable::QuadNode::
QuadNode(int level, const Rect& rect)
	: level(level)
	, rect(rect)
	, mapping_x(0)
	, mapping_y(0)
{
	for (int i = 0; i < 4; ++i) {
		children[i] = nullptr;
	}
}

VirtualTexture::PageTable::Rect VirtualTexture::PageTable::QuadNode::
CalcChildRect(int idx) const
{
	int x = rect.x;
	int y = rect.y;
	int w = rect.w / 2;
	int h = rect.h / 2;

	switch (idx)
	{
	case 0:
		return Rect(x, y, w, h);
	case 1:
		return Rect(x + w, y, w, h);
	case 2:
		return Rect(x + w, y + h, w, h);
	case 3:
		return Rect(x, y + h, w, h);
	default:
		assert(0);
		return rect;
	}
}

void VirtualTexture::PageTable::QuadNode::
Write(int w, int h, uint8_t* data, int mip_level)
{
	if (level < mip_level) {
		return;
	}

	// fill
	int rx = rect.x >> mip_level;
	int ry = rect.y >> mip_level;
	int rw = rect.w >> mip_level;
	int rh = rect.h >> mip_level;
	for (int y = ry; y < ry + rh; ++y) {
		for (int x = rx; x < rx + rw; ++x) {
			int ptr = (y * w + x) * 4;
			data[ptr + 0] = mapping_x;
			data[ptr + 1] = mapping_y;
			data[ptr + 2] = level;
			data[ptr + 3] = 255;
		}
	}

	for (int i = 0; i < 4; ++i) {
		if (children[i] != nullptr) {
			children[i]->Write(w, h, data, mip_level);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// class VirtualTexture::PageCache
//////////////////////////////////////////////////////////////////////////

VirtualTexture::PageCache::
PageCache(TextureAtlas& atlas, PageTable& table, const PageIndexer& indexer, TileDataFile& file)
	: m_atlas(atlas)
	, m_table(table)
	, m_lru(indexer)
	, m_indexer(indexer)
	, m_file(file)
{
	size_t sz = file.GetTileFileSize();
	m_tile_buf = new uint8_t[sz];
	memset(m_tile_buf, 0, sz);
}

VirtualTexture::PageCache::
~PageCache()
{
	delete[] m_tile_buf;
}

void VirtualTexture::PageCache::
LoadComplete(const Page& page, const uint8_t* data)
{
	//printf("++ %d %d, %d\n", page.x, page.y, page.mip);

	int x, y;

	int page_n = m_atlas.GetPageCount();
	if (m_lru.Size() == page_n * page_n)
	{
		auto end = m_lru.GetListEnd();
		assert(end);

		m_table.RemovePage(end->page);

		x = end->x;
		y = end->y;
		m_lru.RemoveBack();
	}
	else
	{
		x = m_lru.Size() % page_n;
		y = m_lru.Size() / page_n;
	}
	m_lru.AddFront(page, x, y);

	m_atlas.UploadPage(data, x, y);

	m_table.AddPage(page, x, y);
}

bool VirtualTexture::PageCache::
Touch(const Page& page)
{
	if (!m_lru.Find(page)) {
		return false;
	} else {
		m_lru.Touch(page);
		return true;
	}
}

bool VirtualTexture::PageCache::
Request(const Page& page)
{
	if (m_lru.Find(page)) {
		return false;
	} else {
		m_file.ReadPage(m_indexer.CalcPageIdx(page), m_tile_buf);
		//CopyBorder(m_tile_buf);
		LoadComplete(page, m_tile_buf);
		return true;
	}
}

void VirtualTexture::PageCache::
CopyBorder(uint8_t* pixels) const
{
	int channel = 4;
	int pagesize = m_atlas.GetPageSize();
	int bordersize = 2;

	float height = 65535.0f;

	for (int y = 0; y < pagesize; ++y) {
		for (int x = 0; x < pagesize; ++x) {
			if (x < bordersize || x >= pagesize - bordersize ||
				y < bordersize || y >= pagesize - bordersize) {
				//for (int i = 0; i < channel; ++i) {
				//	pixels[(y * pagesize + x) * channel + i] = 0;
				//}
				memcpy(&pixels[(y * pagesize + x) * channel], &height, 4);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// class VirtualTexture::PageCache::LRUCollection
//////////////////////////////////////////////////////////////////////////

VirtualTexture::PageCache::LRUCollection::
LRUCollection(const PageIndexer& indexer)
	: m_indexer(indexer)
	, m_list_begin(nullptr)
	, m_list_end(nullptr)
	, m_freelist(nullptr)
{
}

VirtualTexture::PageCache::LRUCollection::
~LRUCollection()
{
	for (auto& itr : m_map) {
		delete itr.second;
	}
}

bool VirtualTexture::PageCache::LRUCollection::
RemoveBack()
{
	if (m_map.empty()) {
		return false;
	}

	int key = m_indexer.CalcPageIdx(m_list_end->page);
	m_map.erase(key);

	assert(m_list_end);

	if (m_list_begin == m_list_end)
	{
		if (m_freelist) {
			m_freelist->prev = m_list_begin;
		}
		m_list_begin->next = m_freelist;

		m_freelist = m_list_begin;
		m_list_begin = nullptr;
		m_list_end = nullptr;
	}
	else
	{
		auto back = m_list_end;
		if (back->prev) {
			back->prev->next = nullptr;
		}
		m_list_end = back->prev;
		m_list_end->next = nullptr;

		back->next = m_freelist;
		if (m_freelist) {
			m_freelist->prev = back;
		}
		m_freelist = back;
	}

#ifdef CHECK_LRU
	Check();
#endif // CHECK_LRU

	return true;
}

bool VirtualTexture::PageCache::LRUCollection::
AddFront(const Page& page, int x, int y)
{
	int idx = m_indexer.CalcPageIdx(page);
	if (m_map.find(idx) != m_map.end()) {
		return false;
	}

	CachePage* cp;
	if (m_freelist) {
		cp = m_freelist;
		m_freelist = m_freelist->next;
		if (m_freelist) {
			m_freelist->prev = nullptr;
		}
	} else {
		cp = new CachePage();
#ifdef CHECK_LRU
		++tot_count;
#endif // CHECK_LRU
	}

	m_map.insert({ idx, cp });

	cp->page = page;
	cp->x = x;
	cp->y = y;
	cp->prev = nullptr;
	cp->next = m_list_begin;
	if (m_list_begin)
	{
		m_list_begin->prev = cp;
		m_list_begin = cp;
	}
	else
	{
		m_list_begin = cp;
		m_list_end = cp;
	}

#ifdef CHECK_LRU
	Check();
#endif // CHECK_LRU

	return true;
}

bool VirtualTexture::PageCache::LRUCollection::
Find(const Page& page) const
{
	return m_map.find(m_indexer.CalcPageIdx(page)) != m_map.end();
}

void VirtualTexture::PageCache::LRUCollection::
Touch(const Page& page)
{
	if (m_map.size() <= 1) {
		return;
	}

	auto itr = m_map.find(m_indexer.CalcPageIdx(page));
	if (itr == m_map.end()) {
		return;
	}

	CachePage* curr = itr->second;

	if (curr == m_list_begin) {
		return;
	}
	if (curr == m_list_end) {
		m_list_end = curr->prev;
	}

	auto prev = curr->prev;
	auto next = curr->next;
	if (prev) {
		prev->next = next;
	}
	if (next) {
		next->prev = prev;
	}

	curr->next = m_list_begin;
	if (m_list_begin) {
		m_list_begin->prev = curr;
	}
	curr->prev = nullptr;
	m_list_begin = curr;

#ifdef CHECK_LRU
	Check();
#endif // CHECK_LRU
}

void VirtualTexture::PageCache::LRUCollection::
Clear()
{
	m_map.clear();

	if (m_list_begin)
	{
		assert(m_list_end);
		m_list_end->next = m_freelist;
		if (m_freelist) {
			m_freelist->prev = m_list_end;
		}
		m_freelist = m_list_begin;

		m_list_begin = nullptr;
		m_list_end = nullptr;
	}

#ifdef CHECK_LRU
	Check();
#endif // CHECK_LRU
}

void VirtualTexture::PageCache::LRUCollection::
Check()
{
#ifdef CHECK_LRU
	assert(!m_list_begin->prev && !m_list_end->next);

	int count = 0;
	auto n = m_list_begin;
	auto last = n;
	while (n) {
		last = n;

		++count;
		assert(count <= m_map.size());

		if (n->next && n->next->prev != n) {
			assert(0);
		}
		if (n->prev && n->prev->next != n) {
			assert(0);
		}

		n = n->next;
	}

	assert(last == m_list_end);
	assert(count == m_map.size());

	int free_count = 0;
	n = m_freelist;
	while (n) {
		++free_count;
		n = n->next;
	}
	assert(free_count + count == tot_count);
#endif // CHECK_LRU
}


//////////////////////////////////////////////////////////////////////////
// struct VirtualTexture::TileDataFile
//////////////////////////////////////////////////////////////////////////

VirtualTexture::TileDataFile::
TileDataFile(const VTexInfo& info, std::fstream& file)
	: m_file(file)
{
	const size_t size = info.tile_size + info.border_size * 2;
	m_tile_file_sz = size * size * info.channels * info.bytes;
}

void VirtualTexture::TileDataFile::
ReadPage(int index, uint8_t* data) const
{
	//std::lock_guard<std::mutex> lock(m_mutex);

	memset(data, 0xff, m_tile_file_sz);

	const uint64_t header_size = sizeof(VTexInfo);
	m_file.seekg(header_size + static_cast<uint64_t>(m_tile_file_sz) * index);
	m_file.read(reinterpret_cast<char*>(data), m_tile_file_sz);
}

void VirtualTexture::TileDataFile::
WritePage(int index, const uint8_t* data)
{
	//std::lock_guard<std::mutex> lock(m_mutex);

	const uint64_t header_size = sizeof(VTexInfo);
	m_file.seekp(header_size + static_cast<uint64_t>(m_tile_file_sz) * index);
	m_file.write(reinterpret_cast<const char*>(data), m_tile_file_sz);
}

}