module;
#include <print>
#include <array>
#include <string>
#include <vector>
#include <fstream>
#include <cstdint>
#include <optional>
#include <string_view>
#include <glm/glm.hpp>
export module scene.plymesh;
import buffers.mesh;
import vulkan_ma_hpp;

export struct Plymesh {
    void init(vma::Allocator vmalloc, std::string_view path_rel, std::optional<glm::vec3> color = std::nullopt) {
        std::ifstream file;
        std::string path_full = path_rel.data();
		file.open(path_full, std::ifstream::binary);
        if (file.good()) {
            std::string line;

            // validate header start
            std::getline(file, line);
            if (line != "ply") {
                std::println("corrupted header for {}", path_full);
            }
            
            // ignoring format for now
            std::getline(file, line);

            // element vertex N
            std::getline(file, line);
            size_t str_pos = line.find_last_of(' ');
            std::string vert_n_str = line.substr(str_pos + 1, line.size() - str_pos);
            size_t vert_n = std::stoi(vert_n_str);
            
            // skip parsing vertex contents for now
            for (size_t i = 0; i < 6; i++) {
                std::getline(file, line);
            }

            // element face N
            std::getline(file, line);
            str_pos = line.find_last_of(' ');
            std::string face_n_str = line.substr(str_pos + 1, line.size() - str_pos);
            size_t face_n = std::stoi(face_n_str);

            // skip face properties
            std::getline(file, line);

            // validate header end
            std::getline(file, line);
            if (line != "end_header") {
                std::println("missing/unexpected header end for {}", path_full);
            }

            // read little endian vertices
            typedef std::pair<glm::vec3, glm::vec3> RawVertex;
            std::vector<RawVertex> raw_vertices(vert_n);
            file.read(reinterpret_cast<char*>(raw_vertices.data()), sizeof(RawVertex) * raw_vertices.size());

            // read little endian faces into indices
            std::vector<uint32_t> raw_indices;
            raw_indices.reserve(face_n * 3);
            for (size_t i = 0; i < face_n; i++) {
                uint8_t indices_n; // number of indices per face
                file.read(reinterpret_cast<char*>(&indices_n), sizeof(uint8_t));
                std::array<int, 3> indices_face;
                file.read(reinterpret_cast<char*>(&indices_face), sizeof(int) * 3);
                raw_indices.insert(raw_indices.end(), indices_face.cbegin(), indices_face.cend());
            }
            file.close();

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
        }
        else {
            std::println("failed to load ply file: {}", path_full);
        }
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