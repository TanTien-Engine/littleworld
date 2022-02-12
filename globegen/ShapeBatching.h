#pragma once

#include <SM_Vector.h>

#include <memory>
#include <vector>

namespace gs { class Shape3D; }

namespace globegen
{

class ShapeBatching
{
public:
	void Add(const std::shared_ptr<gs::Shape3D>& shape, uint32_t col);

	auto& GetBuffer() const { return m_buf; }

public:
	struct Vertex
	{
		sm::vec3 pos;
		uint32_t col = 0;
	};

	struct Buffer
	{
		void Reserve(size_t idx_count, size_t vtx_count);

		void Clear();

		std::vector<Vertex>         vertices;
		std::vector<unsigned short> indices;

		unsigned short  curr_index = 0;
		Vertex*         vert_ptr = nullptr;
		unsigned short* index_ptr = nullptr;
	};

private:
	Buffer m_buf;

}; // ShapeBatching

}