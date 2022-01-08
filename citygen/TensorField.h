#pragma once

#include <SM_Vector.h>

#include <vector>

namespace ur { class Texture; }

namespace citygen
{

class TensorField
{
public:
	TensorField(const ur::Texture& tex);

	auto GetWidth() const { return m_width; }
	auto GetHeight() const { return m_height; }

	sm::vec4 GetTensor(int x, int y) const;

private:
	int m_width = 0;
	int m_height = 0;
	
	std::vector<sm::vec4> m_tensors;

}; // TensorField

}