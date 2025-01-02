#pragma once
#include "chad_vis/entities/mesh/indices.hpp"
#include "chad_vis/entities/mesh/vertices.hpp"

template<typename Vertex, typename Index = uint16_t> 
struct Mesh {
    void init(vma::Allocator vmalloc, const vk::ArrayProxy<uint32_t>& queues, std::span<Vertex> vertices, std::span<Index> indices) {
        _indices.init(vmalloc, queues, indices);
        _vertices.init(vmalloc, queues, vertices);
    }
    void init(vma::Allocator vmalloc, const vk::ArrayProxy<uint32_t>& queues, std::span<Vertex> vertices) {
        _vertices.init(vmalloc, queues, vertices);
    }
    void destroy(vma::Allocator vmalloc) {
        _vertices.destroy(vmalloc);
        if (_indices._count > 0) _indices.destroy(vmalloc);
    }

    Indices<Index> _indices;
    Vertices<Vertex> _vertices;
};