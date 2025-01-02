#pragma once
#include <cstdint>
#include <vk_mem_alloc.hpp>
#include <vulkan/vulkan.hpp>
#include "chad_vis/device/buffer.hpp"


template<typename Index>
struct Indices {
    void init(vma::Allocator vmalloc, const vk::ArrayProxy<uint32_t>& queues, std::span<Index> index_data) {
        // create index buffer and copy indices to it
		dv::Buffer::CreateInfo info {
			.vmalloc = vmalloc,
			.size = sizeof(Index) * index_data.size(),
			.usage = vk::BufferUsageFlagBits::eIndexBuffer,
			.queue_families = queues,
			.host_accessible = true,
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

	dv::Buffer _buffer;
	uint32_t _count;
};