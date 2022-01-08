#include "TensorField.h"

#include <unirender/Texture.h>
#include <unirender/TextureUtility.h>

namespace
{

float i2f(unsigned int i)
{
	float ret;
	memcpy(&ret, &i, sizeof(float));
	return ret;
}

// from glm
float to_f32(unsigned short value)
{
	int s = (value >> 15) & 0x00000001;
	int e = (value >> 10) & 0x0000001f;
	int m =  value        & 0x000003ff;

	if(e == 0)
	{
		if(m == 0)
		{
			//
			// Plus or minus zero
			//

			return i2f(static_cast<unsigned int>(s << 31));
		}
		else
		{
			//
			// Denormalized number -- renormalize it
			//

			while(!(m & 0x00000400))
			{
				m <<= 1;
				e -=  1;
			}

			e += 1;
			m &= ~0x00000400;
		}
	}
	else if(e == 31)
	{
		if(m == 0)
		{
			//
			// Positive or negative infinity
			//

			return i2f(static_cast<unsigned int>((s << 31) | 0x7f800000));
		}
		else
		{
			//
			// Nan -- preserve sign and significand bits
			//

			return i2f(static_cast<unsigned int>((s << 31) | 0x7f800000 | (m << 13)));
		}
	}

	//
	// Normalized number
	//

	e = e + (127 - 15);
	m = m << 13;

	//
	// Assemble s, e and m.
	//

	return i2f(static_cast<unsigned int>((s << 31) | (e << 23) | m));
}

}

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
				t.xyzw[k] = to_f32(pixels[(y * w + x) * 4 + k]);
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