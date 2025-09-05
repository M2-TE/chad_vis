module;
#include <glm/glm.hpp>
#include <cme/detail/asset.hpp>
export module scene.plymesh;
import vulkan.allocator;
import buffers.mesh;
import cme.datasets;

export struct Plymesh {
    void init(vma::Allocator vmalloc, std::string_view path_rel, std::optional<glm::vec3> color = std::nullopt) {
        // if it does not exist, simply quit (no point in going further)
        auto [asset, exists] = datasets::try_load(path_rel);
        if (!exists) {
            std::println("Plymesh not found: {}", path_rel);
            exit(0);
        }

        // validate header
        std::string line;
        const uint8_t* it = asset._data;
        while (*it != '\n') line.push_back(*it++);
        it++;
        if (line != "ply") std::println("corrupted header for {}", path_rel);

        // ignoring format for now
        while (*it != '\n') it++;
        it++;

        // element vertex N
        line.clear();
        while (*it != '\n') line.push_back(*it++);
        it++;
        size_t str_pos = line.find_last_of(' ');
        std::string vert_n_str = line.substr(str_pos + 1, line.size() - str_pos);
        size_t vert_n = std::stoi(vert_n_str);

        // skip parsing vertex contents for now
        for (size_t i = 0; i < 6; i++) {
            while (*it != '\n') it++;
            it++;
        }

        // element face N
        line.clear();
        while (*it != '\n') line.push_back(*it++);
        it++;
        str_pos = line.find_last_of(' ');
        std::string face_n_str = line.substr(str_pos + 1, line.size() - str_pos);
        size_t face_n = std::stoi(face_n_str);

        // skip face properties
        while (*it != '\n') it++;
        it++;

        // validate header end
        line.clear();
        while (*it != '\n') line.push_back(*it++);
        it++;
        if (line != "end_header") std::println("missing/unexpected header end for {}", path_rel);

        // read little endian vertices
        typedef std::pair<glm::vec3, glm::vec3> RawVertex;
        auto beg = reinterpret_cast<const RawVertex*>(it);
        it += sizeof(RawVertex) * vert_n;
        auto end = reinterpret_cast<const RawVertex*>(it);
        std::vector<RawVertex> raw_vertices{ beg, end };

        // read little endian faces into indices
        std::vector<uint32_t> raw_indices;
        raw_indices.reserve(face_n * 3);
        for (size_t face_i = 0; face_i < face_n; face_i++) {
            // skip the number of face indices (single uint8_t)
            it++;

            // read 3 face indices into vector
            for (size_t i = 0; i < 3; i++) {
                const int* index_p = reinterpret_cast<const int*>(it);
                it += sizeof(int);
                raw_indices.push_back(*index_p);
            }
        }

        // flip y and swap y with z and insert into full vertex vector
        std::vector<Vertex> vertices;
        vertices.reserve(vert_n);
        for (auto& vertex: raw_vertices) {
            glm::vec3 pos { vertex.first.x, -vertex.first.z, vertex.first.y };
            glm::vec3 norm { vertex.second.x, -vertex.second.z, vertex.second.y };
            glm::vec3 col = color.has_value() ? glm::vec4(color.value(), 1) : vertex.second;
            vertices.emplace_back(pos, norm, col);
        }

        // convert raw indices to Index type
        std::vector<Index> indices;
        indices.resize(raw_indices.size());
        for (size_t i = 0; i < raw_indices.size(); i++) {
            indices[i] = (Index)raw_indices[i];
        }
        
        // create actual mesh from raw data
        _mesh.init(vmalloc, vertices, indices);
        
        // TODO: validate iterator before performing all those steps
    }
    void destroy(vma::Allocator vmalloc) {
        _mesh.destroy(vmalloc);
    }

    struct Vertex {
        glm::vec3 pos;
        glm::vec3 norm;
        glm::vec3 color;
    };
    typedef uint32_t Index;
    Mesh<Vertex, Index> _mesh;
};