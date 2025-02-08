#pragma once
#include <vk_mem_alloc.hpp>
#include <vulkan/vulkan.hpp>
#include "chad_vis/entities/mesh/indices.hpp"
#include "chad_vis/entities/mesh/vertices.hpp"

template<typename Vertex, typename Index = uint16_t> 
struct Mesh {
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