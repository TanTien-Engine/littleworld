#pragma once

#include <SM_Cube.h>

#include <memory>
#include <vector>
#include <string>

namespace ur { class Device; class VertexArray; }
namespace pm3 { class Polytope; struct TextureMapping; }

namespace archgen
{

class MeshBuilder
{
public:
	static std::shared_ptr<ur::VertexArray> 
		Gen(const ur::Device& dev, const std::vector<std::shared_ptr<pm3::Polytope>>& polys, 
			const std::shared_ptr<pm3::TextureMapping>& uv_map, sm::cube& aabb);
	static std::shared_ptr<ur::VertexArray>
		Gen(const ur::Device& dev, const std::string& filepath, sm::cube& aabb);

}; // MeshBuilder

}