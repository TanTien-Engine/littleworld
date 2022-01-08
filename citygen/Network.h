#pragma once

#include <SM_Vector.h>

#include <vector>
#include <memory>

namespace citygen
{

class TensorField;

class Network
{
public:
	Network(const std::shared_ptr<TensorField>& tf);

	void BuildStreamlines(int num);
	void BuildTopology();

	auto& GetMajorPaths() const { return m_major_paths; }
	auto& GetMinorPaths() const { return m_minor_paths; }

public:
	struct Path
	{
		std::vector<sm::vec2> points;
	};

private:
	std::shared_ptr<Path> BuildPath(const sm::ivec2& p, bool major) const;

	std::vector<sm::vec2> Travel(const sm::ivec2& p, bool major, bool forward) const;
	sm::vec2 CalcDir(const sm::ivec2& p, bool major) const;

private:
	std::shared_ptr<TensorField> m_tf = nullptr;

	std::vector<std::shared_ptr<Path>> m_major_paths, m_minor_paths;

}; // Network

}