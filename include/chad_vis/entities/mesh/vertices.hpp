#pragma once
#include <cstdint>
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.hpp>
#include "chad_vis/device/buffer.hpp"

template<typename Vertex>
struct Vertices {
    void init(vma::Allocator vmalloc, const vk::ArrayProxy<uint32_t>& queues, std::span<Vertex> vertex_data) {
        // create vertex buffer and copy vertices to it
		dv::Buffer::CreateInfo info {
			.vmalloc = vmalloc,
			.size = sizeof(Vertex) * vertex_data.size(),
			.usage = vk::BufferUsageFlagBits::eVertexBuffer,
			.queue_families = queues,
		};
		_buffer.init(info);
		_buffer.write(vmalloc, vertex_data.data(), sizeof(Vertex) * vertex_data.size());
		count = (uint32_t)vertex_data.size();
    }
    void destroy(vma::Allocator vmalloc) {
		_buffer.destroy(vmalloc);
    }

	dv::Buffer _buffer;
	uint32_t count;
};