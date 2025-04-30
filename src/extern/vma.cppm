module;
#include <vk_mem_alloc.h>
#include "vk_mem_alloc.hpp"
export module vma_hpp;

export namespace vma {
    using vma::operator|;
    using vma::operator&;
    using vma::operator^;
    using vma::operator~;
    using vma::functionsFromDispatcher;
    using vma::AllocatorCreateFlagBits;
    using vma::AllocatorCreateFlags;
    using vma::MemoryUsage;
    using vma::AllocationCreateFlagBits;
    using vma::AllocationCreateFlags;
    using vma::PoolCreateFlagBits;
    using vma::PoolCreateFlags;
    using vma::DefragmentationFlagBits;
    using vma::DefragmentationFlags;
    using vma::DefragmentationMoveOperation;
    using vma::VirtualBlockCreateFlagBits;
    using vma::VirtualBlockCreateFlags;
    using vma::VirtualAllocationCreateFlagBits;
    using vma::VirtualAllocationCreateFlags;
    using vma::DeviceMemoryCallbacks;
    using vma::VulkanFunctions;
    using vma::AllocatorCreateInfo;
    using vma::AllocatorInfo;
    using vma::Statistics;
    using vma::DetailedStatistics;
    using vma::TotalStatistics;
    using vma::Budget;
    using vma::AllocationCreateInfo;
    using vma::PoolCreateInfo;
    using vma::AllocationInfo;
    using vma::AllocationInfo2;
    using vma::DefragmentationInfo;
    using vma::DefragmentationMove;
    using vma::DefragmentationPassMoveInfo;
    using vma::DefragmentationStats;
    using vma::VirtualBlockCreateInfo;
    using vma::VirtualAllocationCreateInfo;
    using vma::VirtualAllocationInfo;
    using vma::createAllocator;
    using vma::createVirtualBlock;
    using vma::Allocator;
    using vma::Pool;
    using vma::Allocation;
    using vma::DefragmentationContext;
    using vma::VirtualAllocation;
    using vma::VirtualBlock;
}