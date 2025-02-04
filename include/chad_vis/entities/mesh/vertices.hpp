#pragma once
#include <vk_mem_alloc.hpp>
#include <vulkan/vulkan.hpp>
#include "chad_vis/device/device_buffer.hpp"

template<typename Vertex>
struct Vertices {
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