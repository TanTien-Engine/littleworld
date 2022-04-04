#include "MeshBuilder.h"

#include <unirender/Device.h>
#include <unirender/VertexBuffer.h>
#include <unirender/IndexBuffer.h>
#include <unirender/VertexArray.h>
#include <unirender/ComponentDataType.h>
#include <unirender/VertexInputAttribute.h>
#include <polymesh3/Polytope.h>

#include <../../tinyobjloader/tiny_obj_loader.h>

namespace
{

struct Vertex
{
	sm::vec3 pos;
	sm::vec3 normal;
	sm::vec2 texcoord;
};

std::shared_ptr<ur::VertexArray>
create_va(const ur::Device& dev, const std::vector<Vertex>& vertices, const std::vector<unsigned short>& indices)
{
	if (vertices.empty()) {
		return nullptr;
	}

	auto va = dev.CreateVertexArray();

	auto vbuf_sz = sizeof(Vertex) * vertices.size();
	auto vbuf = dev.CreateVertexBuffer(ur::BufferUsageHint::StaticDraw, vbuf_sz);
	vbuf->ReadFromMemory(vertices.data(), vbuf_sz, 0);
	va->SetVertexBuffer(vbuf);

	if (!indices.empty())
	{
		auto ibuf = dev.CreateIndexBuffer(ur::BufferUsageHint::StaticDraw, 0);
		auto ibuf_sz = sizeof(unsigned short) * indices.size();
		ibuf->SetCount(indices.size());
		ibuf->Reserve(ibuf_sz);
		ibuf->ReadFromMemory(indices.data(), ibuf_sz, 0);
		ibuf->SetDataType(ur::IndexBufferDataType::UnsignedShort);
		va->SetIndexBuffer(ibuf);
	}

	std::vector<std::shared_ptr<ur::VertexInputAttribute>> vbuf_attrs;
    // pos
	vbuf_attrs.push_back(std::make_shared<ur::VertexInputAttribute>(
        0, ur::ComponentDataType::Float, 3, 0, 32
    ));
    // normal
	vbuf_attrs.push_back(std::make_shared<ur::VertexInputAttribute>(
        1, ur::ComponentDataType::Float, 3, 12, 32
    ));
    // texcoord
	vbuf_attrs.push_back(std::make_shared<ur::VertexInputAttribute>(
        2, ur::ComponentDataType::Float, 2, 24, 32
    ));
	va->SetVertexBufferAttrs(vbuf_attrs);

	return va;
}

}

namespace archgen
{

std::shared_ptr<ur::VertexArray> 
MeshBuilder::Gen(const ur::Device& dev, const std::vector<std::shared_ptr<pm3::Polytope>>& polys,
	             const std::shared_ptr<pm3::TextureMapping>& uv_map, sm::cube& aabb)
{
	aabb.MakeEmpty();

	std::vector<Vertex> vertices;
	for (auto& poly : polys)
	{
		auto& points = poly->Points();

		auto& faces = poly->Faces();
		for (auto& f : faces) 
		{
			sm::vec3 normal;
			poly->CalcFaceNormal(*f, normal);

			auto& uv = uv_map ? *uv_map : f->tex_map;

			auto& polyline = f->border;
			if (polyline.size() > 2) 
			{
				for (auto& i : polyline) {
					aabb.Combine(points[i]->pos);
				}

				for (size_t i = 1, n = polyline.size() - 1; i < n; ++i) 
				{
					Vertex tri[3];
					tri[0].pos = points[polyline[0]]->pos;
					tri[1].pos = points[polyline[i]]->pos;
					tri[2].pos = points[polyline[i + 1]]->pos;
					for (auto& p : tri) 
					{
						p.normal = normal;
						p.texcoord = uv.CalcTexCoords(p.pos, 1, 1);

						vertices.push_back(p);
					}
				}
			}
		}
	}

	return create_va(dev, vertices, {});
}

std::shared_ptr<ur::VertexArray>
MeshBuilder::Gen(const ur::Device& dev, const std::string& filepath, sm::cube& aabb)
{
	aabb.MakeEmpty();

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str(), "", true);
	if (!ret) {
		return nullptr;
	}

	std::vector<Vertex> vertices;
	for (size_t s = 0; s < shapes.size(); s++)
	{
		size_t index_offset = 0;

		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
		{
			for (size_t v = 0; v < 3; v++)
			{
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
				tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
				tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
				tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
				tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
				tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
				tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];

				tinyobj::real_t tx, ty;

				if (!attrib.texcoords.empty())
				{
					tx = attrib.texcoords[2 * idx.texcoord_index + 0];
					ty = 1.0 - attrib.texcoords[2 * idx.texcoord_index + 1];
				}
				else
				{
					if (v == 0) {
						tx = ty = 0;
					} else if (v == 1) {
						tx = 0, ty = 1;
					} else {
						tx = ty = 1;
					}
				}

				Vertex vert;
				vert.pos.Set(vx, vy, vz);
				vert.normal.Set(nx, ny, nz);
				vert.texcoord.Set(tx, ty);
				vertices.push_back(vert);

				aabb.Combine(vert.pos);
			}

			index_offset += 3;
		}
	}

	return create_va(dev, vertices, {});
}

}