#include "ShapeBatching.h"

#include <geoshape/Shape3D.h>
#include <geoshape/Line3D.h>
#include <geoshape/Polyline3D.h>

#define PRIMITIVE_RESTART

namespace globegen
{

void ShapeBatching::Add(const std::shared_ptr<gs::Shape3D>& shape, uint32_t col)
{
	auto line = std::dynamic_pointer_cast<gs::Line3D>(shape);
	if (line)
	{
#ifdef PRIMITIVE_RESTART
		m_buf.Reserve(3, 2);
#else
		m_buf.Reserve(2, 2);
#endif // PRIMITIVE_RESTART

		m_buf.vert_ptr[0].pos = line->GetStart();
		m_buf.vert_ptr[0].col = col;
		m_buf.vert_ptr[1].pos = line->GetEnd();
		m_buf.vert_ptr[1].col = col;
		m_buf.vert_ptr += 2;

		m_buf.index_ptr[0] = m_buf.curr_index;
		m_buf.index_ptr[1] = m_buf.curr_index + 1;
#ifdef PRIMITIVE_RESTART
		m_buf.index_ptr[2] = 0xffff;
		m_buf.index_ptr += 3;
#else
		m_buf.index_ptr += 2;
#endif // PRIMITIVE_RESTART
		m_buf.curr_index += 2;

		return;
	}

	auto polyline = std::dynamic_pointer_cast<gs::Polyline3D>(shape);
	if (polyline)
	{
		auto& pts = polyline->GetVertices();
		unsigned short num = static_cast<unsigned short>(pts.size());

		auto is_closed = polyline->GetClosed();

		size_t idx_num = num;
#ifdef PRIMITIVE_RESTART
		idx_num++;
#endif // PRIMITIVE_RESTART
		if (is_closed) {
			idx_num++;
		}

		m_buf.Reserve(idx_num, num);

		for (size_t i = 0; i < num; ++i) {
			m_buf.vert_ptr[i].pos = pts[i];
			m_buf.vert_ptr[i].col = col;
		}
		m_buf.vert_ptr += num;

		for (unsigned short i = 0; i < num; ++i) {
			m_buf.index_ptr[i] = m_buf.curr_index + i;
		}
		if (is_closed) {
			m_buf.index_ptr[num] = m_buf.index_ptr[0];
		}
#ifdef PRIMITIVE_RESTART
		m_buf.index_ptr[idx_num - 1] = 0xffff;
#endif // PRIMITIVE_RESTART
		m_buf.index_ptr += idx_num;
		m_buf.curr_index += num;
	}
}

//////////////////////////////////////////////////////////////////////////
// struct ShapeBatching::Buffer
//////////////////////////////////////////////////////////////////////////

void ShapeBatching::Buffer::Reserve(size_t idx_count, size_t vtx_count)
{
	//assert(!commands.empty());
	//commands.back().elem_count += idx_count;

	size_t sz = vertices.size();
	vertices.resize(sz + vtx_count);
	vert_ptr = vertices.data() + sz;

	sz = indices.size();
	indices.resize(sz + idx_count);
	index_ptr = indices.data() + sz;
}

void ShapeBatching::Buffer::Clear()
{
	//	commands.resize(0);
	vertices.resize(0);
	indices.resize(0);

	curr_index = 0;
	vert_ptr = nullptr;
	index_ptr = nullptr;
}

}