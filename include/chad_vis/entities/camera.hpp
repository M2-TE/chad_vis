#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/type_aligned.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.hpp>
#include "chad_vis/core/input.hpp"
#include "chad_vis/device/buffer.hpp"

struct Camera {
    void init(vma::Allocator vmalloc, const vk::ArrayProxy<uint32_t>& queues) {
        // create camera matrix buffer
		_buffer.init({
            .vmalloc = vmalloc,
            .size = sizeof(glm::aligned_mat4x4),
            .usage = vk::BufferUsageFlagBits::eUniformBuffer,
            .queue_families = queues,
            .host_accessible = true,
		});
    }
    void destroy(vma::Allocator vmalloc) {
		_buffer.destroy(vmalloc);
    }
    
    void resize(vk::Extent2D extent) {
		_extent = extent;
    }
	void update(vma::Allocator vmalloc) {
		// read input for movement and rotation
		float speed = 0.05;
		if (Keys::held(sf::Keyboard::Scan::LControl)) speed /= 8.0;
		if (Keys::held(sf::Keyboard::Scan::LShift)) speed *= 8.0;

		// move in direction relative to camera
		glm::qua<float, glm::aligned_highp> q_rot(_rot);
		if (Keys::held('w')) _pos += q_rot * glm::aligned_vec3(0, 0, +speed);
		if (Keys::held('s')) _pos += q_rot * glm::aligned_vec3(0, 0, -speed);
		if (Keys::held('d')) _pos += q_rot * glm::aligned_vec3(+speed, 0, 0);
		if (Keys::held('a')) _pos += q_rot * glm::aligned_vec3(-speed, 0, 0);
		if (Keys::held('q')) _pos += q_rot * glm::aligned_vec3(0, +speed, 0);
		if (Keys::held('e')) _pos += q_rot * glm::aligned_vec3(0, -speed, 0);

		// only control camera when mouse is captured
		if (Mouse::captured()) {
			_rot += glm::aligned_vec3(-Mouse::delta().y, +Mouse::delta().x, 0) * 0.001f;
		}

		// merge rotation and projection matrices
		glm::aligned_mat4x4 matrix;
		matrix = glm::perspectiveFovLH<float>(glm::radians<float>(_fov), (float)_extent.width, (float)_extent.height, _near, _far);
		matrix = glm::rotate(matrix, -_rot.x, glm::aligned_vec3(1, 0, 0));
		matrix = glm::rotate(matrix, -_rot.y, glm::aligned_vec3(0, 1, 0));
		matrix = glm::translate(matrix, -_pos);
		
		// upload data
		_buffer.write(vmalloc, matrix);
	}

	glm::aligned_vec3 _pos = { 0, 0, 0 };
	glm::aligned_vec3 _rot = { 0, 0, 0 };
	dv::Buffer _buffer;
	vk::Extent2D _extent;
	float _fov = 60;
	float _near = 0.01;
	float _far = 1000.0;
};