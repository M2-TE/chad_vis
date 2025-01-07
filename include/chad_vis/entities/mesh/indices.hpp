#pragma once
#include <vk_mem_alloc.hpp>
#include <vulkan/vulkan.hpp>
#include "chad_vis/device/device_buffer.hpp"

template<typename Index>
struct Indices {
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