#pragma once

#include <SM_Vector.h>
#include <SM_Matrix.h>

#include <memory>
#include <fstream>
#include <mutex>
#include <vector>
#include <functional>
#include <unordered_map>

namespace ur { class Texture; class Framebuffer; class ShaderProgram; class VertexArray; }

namespace globegen
{

class VirtualTexture
{
public:
	VirtualTexture(const char* filepath, size_t vtex_sz, size_t tile_sz, size_t border_sz);

	void Update(const std::shared_ptr<ur::Texture>& heightmap, 
		const sm::mat4& view_proj_mat, const sm::vec2& screen_sz);

	std::shared_ptr<ur::Texture> LoadToTexture() const;

	auto GetTableTex() const { return m_table.GetTexture(); }
	auto GetAtlasTex() const { return m_atlas.GetTexture(); }
	size_t GetVTexSize() const { return m_vtex_sz; }
	size_t GetTileSize() const { return m_tile_sz; }
	size_t GetBorderSize() const { return m_border_sz; }

	auto GetFeedbackTex() const { return m_feedback_buf.GetTexture(); }

	void SetWorldSize(float height_scale, float world_scale);

	bool IsHeightMap() const;

private:
	struct Page
	{
		Page() {}
		Page(int x, int y, int mip);

		bool operator == (const Page& p) const;

		int x = 0;
		int y = 0;
		int mip = 0;
	};

	class PageIndexer
	{
	public:
		PageIndexer(size_t vtex_sz, size_t tile_sz);

		int CalcPageIdx(const Page& page) const;
		const Page& QueryPageByIdx(size_t idx) const;
		int GetPageCount() const { return m_page_count; }

	private:
		int m_page_count;
		int m_mip_count;

		std::vector<int> m_offsets;
		std::vector<sm::ivec2> m_sizes;

		std::vector<Page> m_pages;

	}; // PageIndexer

	class FeedbackBuffer
	{
	public:
		FeedbackBuffer(const PageIndexer& page_idx,
			size_t vtex_sz, size_t tile_sz);
		~FeedbackBuffer();

		std::vector<int> Update(const std::shared_ptr<ur::Texture>& heightmap, 
			const sm::mat4& view_proj_mat, const sm::vec2& screen_sz);

		void DecreaseMipBias();

		std::shared_ptr<ur::Texture> GetTexture() const { return m_tex; }

		void SetWorldSize(float height_scale, float world_scale);

	private:
		void Update(const sm::mat4& view_proj_mat, const sm::vec2& screen_sz);
		std::vector<int> Draw(const std::shared_ptr<ur::Texture>& heightmap, const sm::vec2& screen_sz);

		void Download();

	private:
		const PageIndexer& m_indexer;

		size_t m_page_table_size = 0;

		std::shared_ptr<ur::Texture>       m_tex = nullptr;
		std::shared_ptr<ur::Framebuffer>   m_fbo = nullptr;
		std::shared_ptr<ur::ShaderProgram> m_shader = nullptr;
		std::shared_ptr<ur::VertexArray>   m_feedback_vao = nullptr;

		uint8_t* m_buf = nullptr;

		float m_mip_bias = 0;

		std::vector<int> m_requests;

		float m_height_scale = 0.01f;
		float m_world_scale = 1.0f;

	}; // FeedbackBuffer

	class TextureAtlas
	{
	public:
		TextureAtlas(size_t atlas_sz, size_t page_sz, size_t border_sz);
		~TextureAtlas();

		void UploadPage(const uint8_t* pixels, int x, int y);

		size_t GetPageSize() const { return m_page_sz + m_border_sz * 2; }
		size_t GetPageCount() const { return m_page_count; }

		std::shared_ptr<ur::Texture> GetTexture() const { return m_tex; }

	private:
		size_t m_page_sz;
		size_t m_border_sz;
		size_t m_page_count;

		std::shared_ptr<ur::Texture> m_tex = nullptr;

	}; // TextureAtlas

	class PageTable
	{
	public:
		PageTable(size_t width, size_t height);

		void AddPage(const Page& page, int mapping_x, int mapping_y);
		void RemovePage(const Page& page);

		void Update();

		std::shared_ptr<ur::Texture> GetTexture() const { return m_tex; }

	private:
		size_t CalcMaxLevel() const;

		struct QuadNode;
		QuadNode* FindPage(const Page& page, int& index) const;

	private:
		struct Rect
		{
			Rect(int x, int y, int w, int h)
				: x(x), y(y), w(w), h(h) {}

			bool Contain(int px, int py) const {
				return px >= x && px < x + w
					&& py >= y && py < y + h;
			}

			int x, y;
			int w, h;
		};

		struct QuadNode
		{
			QuadNode(int level, const Rect& rect);

			Rect CalcChildRect(int idx) const;

			void Write(int w, int h, uint8_t* data, int mip_level);

			int level;
			Rect rect;

			int mapping_x, mapping_y;

			std::unique_ptr<QuadNode> children[4];
		};

		struct Image
		{
			~Image() {
				delete[] data;
			}

			int w = 0, h = 0;
			uint8_t* data = nullptr;
		};

	private:
		int m_width, m_height;

		std::unique_ptr<QuadNode> m_root;

		std::vector<Image> m_data;

		std::shared_ptr<ur::Texture> m_tex = nullptr;

	}; // PageTable

	struct PageWithCount
	{
		PageWithCount(const Page& page, int count)
			: page(page), count(count) {}

		Page page;
		int count = 0;
	};

	class TileDataFile
	{
	public:
		TileDataFile(size_t tile_sz, size_t border_sz, std::fstream& file);

		void ReadPage(int index, uint8_t* data) const;
		void WritePage(int index, const uint8_t* data);

		auto GetTileFileSize() const { return m_tile_file_sz; }

	private:
		size_t m_tile_file_sz;

		std::fstream& m_file;

		//std::mutex m_mutex;

	}; // TileDataFile

	class PageCache
	{
	public:
		PageCache(TextureAtlas& atlas, PageTable& table,
			const PageIndexer& indexer, TileDataFile& file);
		~PageCache();

		void LoadComplete(const Page& page, const uint8_t* data);

		bool Touch(const Page& page);

		bool Request(const Page& page);

		void Clear() { m_lru.Clear(); }

	private:
		void CopyBorder(uint8_t* pixels) const;

	private:
		struct CachePage
		{
			Page page;
			int x, y;

			CachePage *prev = nullptr, *next = nullptr;
		};

		class LRUCollection
		{
		public:
			LRUCollection(const PageIndexer& indexer);
			~LRUCollection();

			bool RemoveBack();
			bool AddFront(const Page& page, int x, int y);

			CachePage* GetListEnd() { return m_list_end; }

			bool Find(const Page& page) const;

			void Touch(const Page& page);

			size_t Size() const { return m_map.size(); }

			void Clear();

		private:
			void Check();

		private:
			const PageIndexer& m_indexer;

			std::unordered_map<int, CachePage*> m_map;
			CachePage *m_list_begin, *m_list_end;

			CachePage* m_freelist;

		}; // LRUCollection

	protected:
		const PageIndexer& m_indexer;
		TileDataFile& m_file;

		TextureAtlas& m_atlas;
		PageTable& m_table;

		LRUCollection m_lru;

		uint8_t* m_tile_buf = nullptr;

	}; // PageCache

private:
	size_t m_vtex_sz = 0;
	size_t m_tile_sz = 0;
	size_t m_border_sz = 0;

	PageIndexer m_indexer;
	FeedbackBuffer m_feedback_buf;

	mutable std::fstream m_file;
	TileDataFile m_tiles_file;

	TextureAtlas m_atlas;
	PageTable m_table;

	PageCache m_cache;

	std::vector<PageWithCount> m_toload;

}; // VirtualTexture

}