#include "TensorField.h"

#include <unirender/Texture.h>
#include <unirender/TextureUtility.h>
#include <SM_Math.h>

namespace citygen
{

TensorField::TensorField(const ur::Texture& tex)
{
	auto w = tex.GetWidth();
	auto h = tex.GetHeight();
	auto f = tex.GetFormat();

	int sz = ur::TextureUtility::RequiredSizeInBytes(w, h, f, 4);
	auto pixels = (unsigned short*)tex.WriteToMemory(sz);

	m_width = w;
	m_height = h;

	m_tensors.resize(w * h);
	for (int y = 0; y < h; ++y) 
	{ 
		for (int x = 0; x < w; ++x) 
		{
			sm::vec4 t;
			for (int k = 0; k < 4; ++k) {
				t.xyzw[k] = sm::f16_to_f32(pixels[(y * w + x) * 4 + k]);
			}
			m_tensors[y * w + x] = t;
		}
	}

	delete[] pixels;
}

sm::vec4 TensorField::GetTensor(int x, int y) const
{
	if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
		return sm::vec4();
	} else {
		return m_tensors[y * m_width + x];
	}
}

}