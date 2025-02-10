module;
#include <tuple>
#include <print>
export module device_buffer;
import vulkan_hpp;
import vk_mem_alloc_hpp;

export struct DeviceBuffer {
	struct CreateInfo {
		vma::Allocator vmalloc;
		vk::DeviceSize size;
		vk::DeviceSize alignment = 0;
		vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eUniformBuffer;
		bool dedicated_memory = false;
	};
	void init(const CreateInfo& info) {
		_size = info.size;
		vk::BufferCreateInfo info_buffer {
			.size = info.size,
			.usage = info.usage,
			.sharingMode = vk::SharingMode::eExclusive,
		};
		// add flags to allow host access if requested (ReBAR if available)
		vma::AllocationCreateInfo info_allocation {
			.flags = requires_staging() ? vma::AllocationCreateFlags{} :
				vma::AllocationCreateFlagBits::eHostAccessSequentialWrite,
			.usage = vma::MemoryUsage::eAutoPreferDevice,
			.requiredFlags = vk::MemoryPropertyFlagBits::eDeviceLocal,
			.preferredFlags = requires_staging() ? vk::MemoryPropertyFlags{} :
				vk::MemoryPropertyFlagBits::eHostCached |
				vk::MemoryPropertyFlagBits::eHostVisible |
				vk::MemoryPropertyFlagBits::eHostCoherent,
		};
		// add various flags if requested
		if (info.dedicated_memory) info_allocation.flags |= vma::AllocationCreateFlagBits::eDedicatedMemory;
		// create buffer
		if (info.alignment > 0) {
			std::tie(_data, _allocation) = info.vmalloc.createBufferWithAlignment(info_buffer, info_allocation, info.alignment);
		}
		else {
			std::tie(_data, _allocation) = info.vmalloc.createBuffer(info_buffer, info_allocation);
		}
	}
	void destroy(vma::Allocator vmalloc) {
		vmalloc.destroyBuffer(_data, _allocation);
	}

	void read(vma::Allocator vmalloc, void* data_p, vk::DeviceSize data_size) {
		if (requires_staging()) std::println("ReBAR required, staging buffer not yet implemented");
		vmalloc.copyAllocationToMemory(_allocation, 0, data_p, data_size);
	}
	void write(vma::Allocator vmalloc, void* data_p, vk::DeviceSize data_size) {
		if (requires_staging()) std::println("ReBAR required, staging buffer not yet implemented");
		vmalloc.copyMemoryToAllocation(data_p, _allocation, 0, data_size);
	}
	template<typename T> void read(vma::Allocator vmalloc, T& data) {
		read(vmalloc, &data, sizeof(T));
	}
	template<typename T> void write(vma::Allocator vmalloc, T& data) {
		write(vmalloc, &data, sizeof(T));
	}

	auto static requires_staging() -> bool& {
		static bool require_staging = true;
		return require_staging;
	}
	void static set_staging_requirement(vma::Allocator vmalloc) {
		// to see if ReBAR is available, create a simple 1GiB buffer
		vk::BufferCreateInfo info_buffer {
			.size = 1 << 30,
			.usage = vk::BufferUsageFlagBits::eTransferSrc,
			.sharingMode = vk::SharingMode::eExclusive,
		};
		vma::AllocationCreateInfo info_allocation {
			.flags = 
			vma::AllocationCreateFlagBits::eHostAccessSequentialWrite |
				vma::AllocationCreateFlagBits::eDedicatedMemory |
				vma::AllocationCreateFlagBits::eMapped,
			.usage = vma::MemoryUsage::eAutoPreferDevice,
			.requiredFlags = 
				vk::MemoryPropertyFlagBits::eDeviceLocal,
			.preferredFlags =
				vk::MemoryPropertyFlagBits::eHostCached |
				vk::MemoryPropertyFlagBits::eHostVisible |
				vk::MemoryPropertyFlagBits::eHostCoherent,
		};
		auto [data, allocation] = vmalloc.createBuffer(info_buffer, info_allocation);
		
		// check for host coherency and visibility
		vk::MemoryPropertyFlags props = vmalloc.getAllocationMemoryProperties(allocation);
		requires_staging() = !static_cast<bool>(props & vk::MemoryPropertyFlagBits::eHostVisible);
		std::println("ReBAR available: {}", !requires_staging());
		
		// clean up
		vmalloc.destroyBuffer(data, allocation);
	}

	vk::Buffer _data;
	vma::Allocation _allocation;
	vk::DeviceSize _size;
};