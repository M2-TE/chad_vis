module;
#include <map>
#include <print>
#include <vector>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <string_view>
#include <spirv_reflect.h>
#include <spvrc/spvrc.hpp>
export module pipeline_base;
import device_buffer;
import vulkan_hpp;
import device;
import image;

export struct PipelineBase {
    typedef std::vector<std::tuple<uint32_t /*set*/, uint32_t /*binding*/, vk::SamplerCreateInfo>> SamplerInfos;
    void destroy(Device& device);
    void write_descriptor(Device& device, uint32_t set, uint32_t binding, Image& image, vk::DescriptorType type, vk::Sampler sampler = nullptr);
    void write_descriptor(Device& device, uint32_t set, uint32_t binding, DeviceBuffer& buffer, vk::DescriptorType type, size_t offset = 0);
    auto static get_module_deprecation() -> bool& {
        static bool _shader_modules_deprecated = true;
        return _shader_modules_deprecated;
    }
    void static set_module_deprecation(vk::PhysicalDevice physical_device) {
        auto available_extensions = physical_device.enumerateDeviceExtensionProperties();
        for (auto& available: available_extensions) {
            if (strcmp(vk::KHRMaintenance5ExtensionName, available.extensionName) == 0) {
                get_module_deprecation() = false;
                return;
            }
        }
        get_module_deprecation() = true;
    }
    
protected:
    auto reflect(vk::Device device, const vk::ArrayProxy<std::string_view>& shaderPaths, const SamplerInfos& sampler_infos)
    -> std::pair<vk::VertexInputBindingDescription, std::vector<vk::VertexInputAttributeDescription>>;

protected:
    vk::Pipeline _pipeline;
    vk::PipelineLayout _pipeline_layout;
    vk::DescriptorPool _pool;
    std::vector<vk::DescriptorSet> _desc_sets;
    std::vector<vk::DescriptorSetLayout> _desc_set_layouts;
    std::vector<vk::Sampler> _immutable_samplers;
};

module: private;
auto get_reflections(const vk::ArrayProxy<std::string_view>& shader_paths)
-> std::vector<spv_reflect::ShaderModule> {
	std::vector<spv_reflect::ShaderModule> reflections;
	for (std::string_view path: shader_paths) {
		auto [shader, shader_size] = spvrc::load(path);
		std::vector<uint32_t> shader_words(shader, shader + shader_size);
		reflections.emplace_back(shader_words);
	}
	return reflections;
}
auto get_vertex_desc(std::vector<spv_reflect::ShaderModule>& reflections)
-> std::pair< vk::VertexInputBindingDescription, std::vector<vk::VertexInputAttributeDescription>> {
	vk::VertexInputBindingDescription vertex_input_desc;
    std::vector<vk::VertexInputAttributeDescription> attr_descs;

	// look for vertex attributes in vertex shader stage
	for (auto& reflection: reflections) {
		auto stage = (vk::ShaderStageFlags)reflection.GetShaderStage();
		if (stage != vk::ShaderStageFlagBits::eVertex) continue;

		uint32_t inputs_n = 0;
		auto result = reflection.EnumerateEntryPointInputVariables("main", &inputs_n, nullptr);
		if (result != SPV_REFLECT_RESULT_SUCCESS) std::println("shader reflection error: {}", (uint32_t)result);
		std::vector<SpvReflectInterfaceVariable*> vars(inputs_n);
		result = reflection.EnumerateEntryPointInputVariables("main", &inputs_n, vars.data());
		if (result != SPV_REFLECT_RESULT_SUCCESS) std::println("shader reflection error: {}", (uint32_t)result);

        // build bind descriptions 
		vertex_input_desc = {
            .binding = 0,
            .stride = 0,
            .inputRate = vk::VertexInputRate::eVertex,
        };

        // gather attribute descriptions
		attr_descs.reserve(vars.size());
		for (auto* input: vars) {
			if (input->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN) continue;
            vk::VertexInputAttributeDescription attrDesc {
                .location = input->location,
                .binding = vertex_input_desc.binding,
                .format = (vk::Format)input->format,
                .offset = 0, // computed later
            };
			attr_descs.push_back(attrDesc);
		}

		// sort attributes by location
        auto sorter = [](auto& a, auto& b) { return a.location < b.location; };
		std::sort(std::begin(attr_descs), std::end(attr_descs), sorter);
		// compute final offsets of each attribute and total vertex stride
		for (auto& attribute: attr_descs) {
			attribute.offset = vertex_input_desc.stride;
			vertex_input_desc.stride += vk::blockSize(attribute.format);;
		}
	}
	return std::make_pair(vertex_input_desc, attr_descs);
}
auto get_reflected_bindings(spv_reflect::ShaderModule& reflection)
-> std::vector<SpvReflectDescriptorBinding*> {
	SpvReflectResult result;

	// get number of descriptor bindings
	uint32_t bindings_n = 0;
	result = reflection.EnumerateEntryPointDescriptorBindings("main", &bindings_n, nullptr);
	if (result != SPV_REFLECT_RESULT_SUCCESS) std::println("shader reflection error: {}", (uint32_t)result);

	// fill vector with reflected descriptor sets
	std::vector<SpvReflectDescriptorBinding*> bindings { bindings_n };
	result = reflection.EnumerateEntryPointDescriptorBindings("main", &bindings_n, bindings.data());
	if (result != SPV_REFLECT_RESULT_SUCCESS) std::println("shader reflection error: {}", (uint32_t)result);
	return bindings;
}
auto get_unique_sets(std::vector<spv_reflect::ShaderModule>& reflections, std::map<std::pair<uint32_t, uint32_t>, vk::Sampler>& sampler_map)
-> std::map<uint32_t /*set*/, std::map<uint32_t /*binding*/, vk::DescriptorSetLayoutBinding>> {
	std::map<uint32_t /*set*/, std::map<uint32_t /*binding*/, vk::DescriptorSetLayoutBinding>> unique_sets;
	for (auto& reflection: reflections) {
		auto reflected_bindings = get_reflected_bindings(reflection);
		auto reflected_stage = (vk::ShaderStageFlags)reflection.GetShaderStage();

		// go over each binding within this shader
		for (auto* reflected_binding_p: reflected_bindings) {
			// insert set if not present
			auto [unique_bindings_it, _] = unique_sets.emplace(reflected_binding_p->set, std::map<uint32_t, vk::DescriptorSetLayoutBinding>());
			auto& unique_bindings = unique_bindings_it->second;

			// insert binding if not present
			auto [binding_it, binding_unique] = unique_bindings.emplace(reflected_binding_p->binding, vk::DescriptorSetLayoutBinding {
				.binding = reflected_binding_p->binding,
				.descriptorType = (vk::DescriptorType)reflected_binding_p->descriptor_type,
				.descriptorCount = reflected_binding_p->count,
				.stageFlags = reflected_stage,
				.pImmutableSamplers = nullptr
			});
			auto& binding = binding_it->second;

			// std::println("\tset {} | binding {}: {} {}", 
            //    reflected_binding_p->set,
            //    reflected_binding_p->binding,
            //    vk::to_string((vk::DescriptorType)reflected_binding_p->descriptor_type),
            //    reflected_binding_p->name);

			// update stage flag if binding already existed
			if (!binding_unique) {
				assert(binding.descriptorType == (vk::DescriptorType)reflected_binding_p->descriptor_type
					&& "descriptor type mismatch");
				assert(binding.descriptorCount == reflected_binding_p->count
					&& "descriptor count mismatch");
				binding.stageFlags |= reflected_stage;
			}
			// assign immutable sampler
			else if (reflected_binding_p->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
				auto sampler_it = sampler_map.find(std::make_pair(reflected_binding_p->set, reflected_binding_p->binding));
				binding.pImmutableSamplers = &sampler_it->second;
			}
		}
	}
	return unique_sets;
}
auto create_sampler_map(vk::Device device,
	const std::vector<std::tuple<uint32_t, uint32_t, vk::SamplerCreateInfo>>& sampler_infos,
	std::vector<spv_reflect::ShaderModule>& reflections,
	std::vector<vk::Sampler>& immutable_samplers)
-> std::map<std::pair<uint32_t, uint32_t>, vk::Sampler> {
	std::map<std::pair<uint32_t, uint32_t>, vk::Sampler> sampler_map;
	for (auto& reflection: reflections) {
		auto reflected_bindings = get_reflected_bindings(reflection);

		// go over all bindings within this shader stage
		for (auto* reflected_binding_p: reflected_bindings) {
			if (reflected_binding_p->descriptor_type != SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) continue;

			// check if this immutable sampler was specified further
			uint32_t set = reflected_binding_p->set;
			uint32_t binding = reflected_binding_p->binding;
			uint32_t sampler_info_i = std::numeric_limits<uint32_t>::max();
			for (uint32_t i = 0; i < sampler_infos.size(); i++) {
				auto& sampler_info = sampler_infos[i];
				// check if set and binding match
				if (std::get<0>(sampler_info) == set && std::get<1>(sampler_info) == binding) {
					sampler_info_i = i;
					break;
				}
			}
			
			// create immutable sampler as per parameter if present
			if (sampler_info_i < std::numeric_limits<uint32_t>::max()) {
				auto& sampler_info = std::get<2>(sampler_infos[sampler_info_i]);
				immutable_samplers.push_back(device.createSampler(sampler_info));
				sampler_map.emplace(std::make_pair(set, binding), immutable_samplers.back());
			}
			// create default immutable sampler
			else {
				immutable_samplers.push_back(device.createSampler({
					.magFilter = vk::Filter::eLinear,
					.minFilter = vk::Filter::eLinear,
					.mipmapMode = vk::SamplerMipmapMode::eLinear,
					.addressModeU = vk::SamplerAddressMode::eClampToEdge,
					.addressModeV = vk::SamplerAddressMode::eClampToEdge,
					.addressModeW = vk::SamplerAddressMode::eClampToEdge,
					.mipLodBias = 0.0f,
					.anisotropyEnable = vk::False,
					.maxAnisotropy = 1.0f,
					.compareEnable = vk::False,
					.compareOp = vk::CompareOp::eAlways,
					.minLod = 0.0f,
					.maxLod = vk::LodClampNone,
					.borderColor = vk::BorderColor::eIntOpaqueBlack,
					.unnormalizedCoordinates = vk::False,
				}));
				sampler_map.emplace(std::make_pair(set, binding), immutable_samplers.back());
			}
		}
	}
	return sampler_map;
}
auto create_set_layouts(vk::Device device, std::map<uint32_t, std::map<uint32_t, vk::DescriptorSetLayoutBinding>>& unique_sets)
-> std::vector<vk::DescriptorSetLayout> {
	std::vector<vk::DescriptorSetLayout> desc_set_layouts;
	for (auto [_, unique_bindings]: unique_sets) {

		// copy bindings into vector
		std::vector<vk::DescriptorSetLayoutBinding> bindings;
		bindings.reserve(unique_bindings.size());
		for (auto [_, binding]: unique_bindings) {
			bindings.push_back(binding);
		}
		
		// create descriptor set layout
		desc_set_layouts.emplace_back(device.createDescriptorSetLayout({
			.bindingCount = (uint32_t)bindings.size(),
			.pBindings = bindings.data()
		}));
	}
	return desc_set_layouts;
}
auto create_descriptor_pool(vk::Device device, std::map<uint32_t, std::map<uint32_t, vk::DescriptorSetLayoutBinding>>& unique_sets)
-> vk::DescriptorPool {
	// count occurance for each descriptor type
	std::map<vk::DescriptorType, uint32_t> tallied_bindings;
	for (auto& [_, unique_bindings]: unique_sets) {
		for (auto& [_, binding]: unique_bindings) {
			// increment descriptor type count
			auto [it_node, tally_unique] = tallied_bindings.emplace(binding.descriptorType, binding.descriptorCount);
			it_node->second += binding.descriptorCount;
		}
	}
	// create descriptor pool based on tallied bindings
	std::vector<vk::DescriptorPoolSize> pool_sizes;
	pool_sizes.reserve(tallied_bindings.size());
    for (auto [binding, count]: tallied_bindings) pool_sizes.emplace_back(binding, count);
    vk::DescriptorPool pool = device.createDescriptorPool({
		.maxSets = (uint32_t)unique_sets.size(),
		.poolSizeCount = (uint32_t)pool_sizes.size(), 
		.pPoolSizes = pool_sizes.data(),
	});
	return pool;
}

void PipelineBase::destroy(Device& device) {
	device._logical.destroyPipeline(_pipeline);
	device._logical.destroyPipelineLayout(_pipeline_layout);

	for (auto& layout: _desc_set_layouts) device._logical.destroyDescriptorSetLayout(layout);
	for (auto& sampler: _immutable_samplers) device._logical.destroySampler(sampler);
	device._logical.destroyDescriptorPool(_pool);
	_desc_sets.clear();
	_desc_set_layouts.clear();
	_immutable_samplers.clear();
}
void PipelineBase::write_descriptor(Device& device, uint32_t set, uint32_t binding, Image& image, vk::DescriptorType type, vk::Sampler sampler) {
	vk::DescriptorImageInfo info_image {
		.sampler = sampler,
		.imageView = image._view,
	};
	switch (type) {
		case vk::DescriptorType::eStorageImage:
			info_image.imageLayout = vk::ImageLayout::eGeneral;
			break;
		case vk::DescriptorType::eSampledImage:
		case vk::DescriptorType::eCombinedImageSampler:
			info_image.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			break;
		default:
			assert(false && "Invalid descriptor type");
			break;
	}
	vk::WriteDescriptorSet write_image {
		.dstSet = _desc_sets[set],
		.dstBinding = binding,
		.descriptorCount = 1,
		.descriptorType = type,
		.pImageInfo = &info_image,
	};
	device._logical.updateDescriptorSets(write_image, {});
}
void PipelineBase::write_descriptor(Device& device, uint32_t set, uint32_t binding, DeviceBuffer& buffer, vk::DescriptorType type, size_t offset) {
	vk::DescriptorBufferInfo info_buffer {
		.buffer = buffer._data,
		.offset = offset,
		.range = buffer._size,
	};
	vk::WriteDescriptorSet write_buffer {
		.dstSet = _desc_sets[set],
		.dstBinding = binding,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = type,
		.pBufferInfo = &info_buffer
	};
	device._logical.updateDescriptorSets(write_buffer, {});
}
auto PipelineBase::reflect(vk::Device device, const vk::ArrayProxy<std::string_view>& shader_paths, const SamplerInfos& sampler_infos)
-> std::pair< vk::VertexInputBindingDescription, std::vector<vk::VertexInputAttributeDescription>> {
	// create shader reflections
	auto reflections = get_reflections(shader_paths);

	// get vertex attributes from vertex shader stage
	auto [vertex_input_desc, attr_descs] = get_vertex_desc(reflections);

	// stop when there are no bindings
	uint32_t binding_count = 0;
	for (auto& reflection: reflections) binding_count += (uint32_t)get_reflected_bindings(reflection).size();
	if (binding_count == 0) return std::make_pair(vertex_input_desc, attr_descs);

	// create samplers and descriptor sets
	auto sampler_map = create_sampler_map(device, sampler_infos, reflections, _immutable_samplers);
	auto unique_sets = get_unique_sets(reflections, sampler_map);
	_desc_set_layouts = create_set_layouts(device, unique_sets);

    // create descriptor pool and allocate descriptor sets from it
	_pool = create_descriptor_pool(device, unique_sets);
    _desc_sets = device.allocateDescriptorSets({
		.descriptorPool = _pool,
		.descriptorSetCount = (uint32_t)_desc_set_layouts.size(),
		.pSetLayouts = _desc_set_layouts.data(),
	});
    return std::make_pair(vertex_input_desc, attr_descs);
}