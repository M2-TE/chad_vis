export module buffers.mesh;
import buffers.device;
import vulkan_hpp;
import vulkan_ma_hpp;

export template<typename Index> struct Indices {
    void init(vma::Allocator vmalloc, std::span<Index> index_data) {
        // create index buffer and copy indices to it
		DeviceBuffer::CreateInfo info {
			.vmalloc = vmalloc,
			.size = sizeof(Index) * index_data.size(),
			.usage = vk::BufferUsageFlagBits::eIndexBuffer,
			.dedicated_memory = true,
		};
		_buffer.init(info);
		_buffer.write(vmalloc, index_data.data(), info.size);
        _count = (uint32_t)index_data.size();
    }
    void destroy(vma::Allocator vmalloc) {
		_buffer.destroy(vmalloc);
    }
	auto get_type() -> vk::IndexType {
		return vk::IndexTypeValue<Index>::value;
	}

	DeviceBuffer _buffer;
	uint32_t _count;
};

export template<typename Vertex> struct Vertices {
    void init(vma::Allocator vmalloc, std::span<Vertex> vertex_data) {
        // create vertex buffer and copy vertices to it
		DeviceBuffer::CreateInfo info {
			.vmalloc = vmalloc,
			.size = sizeof(Vertex) * vertex_data.size(),
			.usage = vk::BufferUsageFlagBits::eVertexBuffer,
			.dedicated_memory = true,
		};
		_buffer.init(info);
		_buffer.write(vmalloc, vertex_data.data(), info.size);
		_count = (uint32_t)vertex_data.size();
    }
    void destroy(vma::Allocator vmalloc) {
		_buffer.destroy(vmalloc);
    }

	DeviceBuffer _buffer;
	uint32_t _count;
};

export template<typename Vertex, typename Index = uint16_t> struct Mesh {
    void init(vma::Allocator vmalloc, std::span<Vertex> vertices, std::span<Index> indices) {
        _vertices.init(vmalloc, vertices);
        _indices.init(vmalloc, indices);
    }
    void init(vma::Allocator vmalloc, std::span<Vertex> vertices) {
        _vertices.init(vmalloc, vertices);
    }
    void destroy(vma::Allocator vmalloc) {
        _vertices.destroy(vmalloc);
        if (_indices._count > 0) _indices.destroy(vmalloc);
    }

    Vertices<Vertex> _vertices;
    Indices<Index> _indices;
};