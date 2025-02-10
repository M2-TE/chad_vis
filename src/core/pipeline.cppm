module;
#include <map>
#include <print>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <spvrc/spvrc.hpp>
#include <spirv_reflect.h>
export module pipeline;
import device_buffer;
import vulkan_hpp;
import device;
import image;
import mesh;

struct PipelineBase {
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
export struct Compute: public PipelineBase {
	struct CreateInfo;
	void init(const CreateInfo& info);
	void execute(vk::CommandBuffer cmd, uint32_t nx, uint32_t ny, uint32_t nz);
};
export struct Graphics: public PipelineBase {
	struct CreateInfo;
	void init(const CreateInfo& info);
	
	// draw fullscreen triangle with color and depth attachments
	void execute(vk::CommandBuffer cmd,
				 Image& color, vk::AttachmentLoadOp color_load,
				 DepthStencil& depth_stencil, vk::AttachmentLoadOp depth_stencil_load);
	// draw fullscreen  triangle with only color attachment
	void execute(vk::CommandBuffer cmd, Image& color_dst, vk::AttachmentLoadOp color_load);

	// draw mesh
	template<typename Vertex, typename Index>
	void execute(vk::CommandBuffer cmd,
			Image& color, vk::AttachmentLoadOp color_load,
			DepthStencil& depth_stencil, vk::AttachmentLoadOp depth_stencil_load,
			Mesh<Vertex, Index>& mesh) {
		vk::RenderingAttachmentInfo info_color {
			.imageView = color._view,
			.imageLayout = color._last_layout,
			.resolveMode = 	vk::ResolveModeFlagBits::eNone,
			.loadOp = color_load,
			.storeOp = vk::AttachmentStoreOp::eStore,
			.clearValue { .color { std::array<float, 4>{ 0, 0, 0, 0 } } }
		};
		vk::RenderingAttachmentInfo info_depth_stencil {
			.imageView = depth_stencil._view,
			.imageLayout = depth_stencil._last_layout,
			.resolveMode = 	vk::ResolveModeFlagBits::eNone,
			.loadOp = depth_stencil_load,
			.storeOp = vk::AttachmentStoreOp::eStore,
			.clearValue = { .depthStencil { .depth = 1.0f, .stencil = 0 } },
		};
		vk::RenderingInfo info_render {
			.renderArea = _render_area,
			.layerCount = 1,
			.colorAttachmentCount = 1,
			.pColorAttachments = &info_color,
			.pDepthAttachment = _depth_enabled ? &info_depth_stencil : nullptr,
			.pStencilAttachment = _stencil_enabled ? &info_depth_stencil : nullptr,
		};
		cmd.beginRendering(info_render);
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline);
		if (_desc_sets.size() > 0) {
			cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipeline_layout, 0, _desc_sets, {});
		}
		// draw beg //
		if (mesh._indices._count > 0) {
			cmd.bindVertexBuffers(0, mesh._vertices._buffer._data, { 0 });
			cmd.bindIndexBuffer(mesh._indices._buffer._data, 0, mesh._indices.get_type());
			cmd.drawIndexed(mesh._indices._count, 1, 0, 0, 0);
		}
		else {
			cmd.bindVertexBuffers(0, mesh._vertices._buffer._data, { 0 });
			cmd.draw(mesh._vertices._count, 1, 0, 0);
		}
		// draw end //
		cmd.endRendering();
	}
	// draw mesh without depth attachment
	template<typename Vertex, typename Index>
	void execute(vk::CommandBuffer cmd,
			Image& color_dst, vk::AttachmentLoadOp color_load,
			Mesh<Vertex, Index>& mesh) {
		vk::RenderingAttachmentInfo info_color_attach {
			.imageView = color_dst._view,
			.imageLayout = color_dst._last_layout,
			.resolveMode = 	vk::ResolveModeFlagBits::eNone,
			.loadOp = color_load,
			.storeOp = vk::AttachmentStoreOp::eStore,
			.clearValue { .color { std::array<float, 4>{ 0, 0, 0, 0 } } }
		};
		vk::RenderingInfo info_render {
			.renderArea = _render_area,
			.layerCount = 1,
			.colorAttachmentCount = 1,
			.pColorAttachments = &info_color_attach,
			.pDepthAttachment = nullptr,
			.pStencilAttachment = nullptr,
		};
		cmd.beginRendering(info_render);
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline);
		if (_desc_sets.size() > 0) {
			cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipeline_layout, 0, _desc_sets, {});
		}
		// draw beg //
		if (mesh._indices._count > 0) {
			cmd.bindVertexBuffers(0, mesh._vertices._buffer._data, { 0 });
			cmd.bindIndexBuffer(mesh._indices._buffer._data, 0, mesh._indices.get_type());
			cmd.drawIndexed(mesh._indices._count, 1, 0, 0, 0);
		}
		else {
			cmd.bindVertexBuffers(0, mesh._vertices._buffer._data, { 0 });
			cmd.draw(mesh._vertices._vertex_n, 1, 0, 0);
		}
		// draw end //
		cmd.endRendering();
	}

private:
	vk::Rect2D _render_area;
	bool _depth_enabled;
	bool _stencil_enabled;
};

struct Compute::CreateInfo {
	const Device& device;
	std::string_view cs_path; vk::SpecializationInfo spec_info = {};
	// TODO: deprecate this one
	SamplerInfos sampler_infos = {};
};
struct Graphics::CreateInfo {
	const Device& device;
	vk::Extent2D extent;
	//
	std::string_view vs_path; vk::SpecializationInfo vs_spec = {};
	std::string_view fs_path; vk::SpecializationInfo fs_spec = {};
	//
	struct Color {
		vk::ArrayProxy<vk::Format> formats;
		vk::Bool32 blend = false;
	} color = {};
	struct Depth {
		vk::Format format = vk::Format::eUndefined;
		vk::Bool32 write = false;
		vk::Bool32 test = false;
	} depth = {};
	struct Stencil {
		vk::Format format = vk::Format::eUndefined;
		vk::Bool32 test = false;
		vk::StencilOpState front = {};
		vk::StencilOpState back = {};
	} stencil = {};
	//
	vk::ArrayProxy<vk::DynamicState> dynamic_states = {};
	// TODO: deprecate this one
	SamplerInfos sampler_infos = {};
};

module: private;
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
void Compute::init(const CreateInfo& info) {
	// reflect shader contents
	reflect(info.device._logical, info.cs_path, info.sampler_infos);

	// create pipeline layout
	_pipeline_layout = info.device._logical.createPipelineLayout({
		.setLayoutCount = (uint32_t)_desc_set_layouts.size(),
		.pSetLayouts = _desc_set_layouts.data(),
	});

	// create pipeline
	auto [cs_code, cs_size] = spvrc::load(info.cs_path);
	vk::ShaderModuleCreateInfo info_cs {
		.codeSize = cs_size * sizeof(uint32_t),
		.pCode = cs_code,
	};
	// optionally create shader module in the deprecated way
	vk::ShaderModule cs_module = nullptr;
	if (get_module_deprecation()) cs_module = info.device._logical.createShaderModule(info_cs);

	vk::ComputePipelineCreateInfo info_compute_pipe {
		.stage = {
			.pNext = get_module_deprecation() ? nullptr : &info_cs,
			.stage = vk::ShaderStageFlagBits::eCompute,
			.module = get_module_deprecation() ? cs_module : nullptr,
			.pName = "main",
			.pSpecializationInfo = info.spec_info.dataSize > 0 ? &info.spec_info : nullptr,
		},
		.layout = _pipeline_layout,
	};
	auto [result, pipeline] = info.device._logical.createComputePipeline(nullptr, info_compute_pipe);
	if (result != vk::Result::eSuccess) std::println("error creating compute pipeline");
	_pipeline = pipeline;
	// destroy shader module if previously created
	if (get_module_deprecation()) info.device._logical.destroyShaderModule(cs_module);
}
void Compute::execute(vk::CommandBuffer cmd, uint32_t nx, uint32_t ny, uint32_t nz) {
	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, _pipeline);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipeline_layout, 0, _desc_sets, {});
	cmd.dispatch(nx, ny, nz);
}
void Graphics::init(const CreateInfo& info) {
	// reflect shader contents
	auto [bind_desc, attr_descs] = reflect(info.device._logical, { info.vs_path, info.fs_path }, info.sampler_infos);

	// create pipeline layout
	vk::PipelineLayoutCreateInfo layoutInfo {
		.setLayoutCount = (uint32_t)_desc_set_layouts.size(),
		.pSetLayouts = _desc_set_layouts.data()
	};
	_pipeline_layout = info.device._logical.createPipelineLayout(layoutInfo);

	// create shader stages
	auto [vs_code, vs_size] = spvrc::load(info.vs_path);
	auto [fs_code, fs_size] = spvrc::load(info.fs_path);
	vk::ShaderModuleCreateInfo info_vs {
		.codeSize = vs_size * sizeof(uint32_t),
		.pCode = vs_code,
	};
	vk::ShaderModuleCreateInfo info_fs {
		.codeSize = fs_size * sizeof(uint32_t),
		.pCode = fs_code,
	};
	// optionally create shader modules in the deprecated way
	vk::ShaderModule vs_module = nullptr;
	vk::ShaderModule fs_module = nullptr;
	if (get_module_deprecation()) vs_module = info.device._logical.createShaderModule(info_vs);
	if (get_module_deprecation()) fs_module = info.device._logical.createShaderModule(info_fs);
	std::array<vk::PipelineShaderStageCreateInfo, 2> shader_stages {{
		vk::PipelineShaderStageCreateInfo {
			.pNext = get_module_deprecation() ? nullptr : &info_vs,
			.stage = vk::ShaderStageFlagBits::eVertex,
			.module = get_module_deprecation() ? vs_module : nullptr,
			.pName = "main",
			.pSpecializationInfo = info.vs_spec.dataSize > 0 ? &info.vs_spec : nullptr,
		},
		vk::PipelineShaderStageCreateInfo {
			.pNext = get_module_deprecation() ? nullptr : &info_fs,
			.stage = vk::ShaderStageFlagBits::eFragment,
			.module = get_module_deprecation() ? fs_module : nullptr,
			.pName = "main",
			.pSpecializationInfo = info.fs_spec.dataSize > 0 ? &info.fs_spec : nullptr,
		}
	}};

	// create graphics pipeline parts
	vk::PipelineVertexInputStateCreateInfo info_vertex_input;
	if (attr_descs.size() > 0) {
		info_vertex_input = {
			.vertexBindingDescriptionCount = 1,
			.pVertexBindingDescriptions = &bind_desc,
			.vertexAttributeDescriptionCount = (uint32_t)attr_descs.size(),
			.pVertexAttributeDescriptions = attr_descs.data(),
		};
	}
	vk::PipelineInputAssemblyStateCreateInfo info_input_assembly {
		.topology = vk::PrimitiveTopology::eTriangleList,
		.primitiveRestartEnable = vk::False,
	};
	vk::PipelineTessellationStateCreateInfo info_tessellation {
		.patchControlPoints = 0, // not using tesselation
	};
	vk::Viewport viewport {
		.x = 0, .y = 0,
		.width = (float)info.extent.width,
		.height = (float)info.extent.height,
		.minDepth = 0.0,
		.maxDepth = 1.0,
	};
	vk::Rect2D scissor({0, 0}, info.extent);
	vk::PipelineViewportStateCreateInfo info_viewport {
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = & scissor,	
	};
	vk::PipelineRasterizationStateCreateInfo info_rasterization {
		.depthClampEnable = false,
		.rasterizerDiscardEnable = false,
		.polygonMode = vk::PolygonMode::eFill,
		.cullMode = vk::CullModeFlagBits::eBack,
		.frontFace = vk::FrontFace::eClockwise,
		.depthBiasEnable = false,
		.lineWidth = 1.0,
	};
	vk::PipelineMultisampleStateCreateInfo info_multisampling {
		.sampleShadingEnable = false,
		.alphaToCoverageEnable = false,
		.alphaToOneEnable = false,
	};
	vk::PipelineDepthStencilStateCreateInfo info_depth_stencil {
		.depthTestEnable = info.depth.test,
		.depthWriteEnable = info.depth.write,
		.depthCompareOp = vk::CompareOp::eLessOrEqual,
		.depthBoundsTestEnable = false,
		.stencilTestEnable = info.stencil.test,
		.front = info.stencil.front,
		.back = info.stencil.back,
	};
	vk::PipelineColorBlendAttachmentState info_blend_attach {
		.blendEnable = info.color.blend,
		.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
		.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
		.colorBlendOp = vk::BlendOp::eAdd,
		.srcAlphaBlendFactor = vk::BlendFactor::eOne,
		.dstAlphaBlendFactor = vk::BlendFactor::eZero,
		.alphaBlendOp = vk::BlendOp::eAdd,
		.colorWriteMask = 
			vk::ColorComponentFlagBits::eR |
			vk::ColorComponentFlagBits::eG |
			vk::ColorComponentFlagBits::eB,
	};
	vk::PipelineColorBlendStateCreateInfo info_blend_state {
		.logicOpEnable = false,
		.attachmentCount = 1,
		.pAttachments = &info_blend_attach,	
		.blendConstants = std::array<float, 4>{ 1.0, 1.0, 1.0, 1.0 },
	};
	vk::PipelineDynamicStateCreateInfo info_dynamic_state {
		.dynamicStateCount = (uint32_t)info.dynamic_states.size(),
		.pDynamicStates = info.dynamic_states.data(),
	};

	// create pipeline
	vk::PipelineRenderingCreateInfo renderInfo {
		.colorAttachmentCount = (uint32_t)info.color.formats.size(),
		.pColorAttachmentFormats = info.color.formats.data(),
		.depthAttachmentFormat = info.depth.format,
		.stencilAttachmentFormat = info.stencil.format,
	};
	vk::GraphicsPipelineCreateInfo pipeInfo {
		.pNext = &renderInfo,
		.stageCount = (uint32_t)shader_stages.size(), 
		.pStages = shader_stages.data(),
		.pVertexInputState = &info_vertex_input,
		.pInputAssemblyState = &info_input_assembly,
		.pTessellationState = &info_tessellation,
		.pViewportState = &info_viewport,
		.pRasterizationState = &info_rasterization,
		.pMultisampleState = &info_multisampling,
		.pDepthStencilState = &info_depth_stencil,
		.pColorBlendState = &info_blend_state,
		.pDynamicState = &info_dynamic_state,
		.layout = _pipeline_layout,
	};

	auto [result, pipeline] = info.device._logical.createGraphicsPipeline(nullptr, pipeInfo);
	if (result != vk::Result::eSuccess) std::println("error creating graphics pipeline");
	_pipeline = pipeline;
	// set persistent options
	_render_area = vk::Rect2D({ 0,0 }, info.extent);
	_depth_enabled = info.depth.test || info.depth.write;
	_stencil_enabled = info.stencil.test;
	// destroy shader modules if previously created
	if (get_module_deprecation()) info.device._logical.destroyShaderModule(vs_module);
	if (get_module_deprecation()) info.device._logical.destroyShaderModule(fs_module);
}
void Graphics::execute(vk::CommandBuffer cmd,
		Image& color, vk::AttachmentLoadOp color_load,
		DepthStencil& depth_stencil, vk::AttachmentLoadOp depth_stencil_load) {
	vk::RenderingAttachmentInfo info_color {
		.imageView = color._view,
		.imageLayout = color._last_layout,
		.resolveMode = 	vk::ResolveModeFlagBits::eNone,
		.loadOp = color_load,
		.storeOp = vk::AttachmentStoreOp::eStore,
		.clearValue { .color { std::array<float, 4>{ 0, 0, 0, 0 } } }
	};
	vk::RenderingAttachmentInfo info_depth_stencil {
		.imageView = depth_stencil._view,
		.imageLayout = depth_stencil._last_layout,
		.resolveMode = 	vk::ResolveModeFlagBits::eNone,
		.loadOp = depth_stencil_load,
		.storeOp = vk::AttachmentStoreOp::eStore,
		.clearValue { .depthStencil { .depth = 1.0f, .stencil = 0 } },
	};
	vk::RenderingInfo info_render {
		.renderArea = _render_area,
		.layerCount = 1,
		.colorAttachmentCount = 1,
		.pColorAttachments = &info_color,
		.pDepthAttachment = _depth_enabled ? &info_depth_stencil : nullptr,
		.pStencilAttachment = _stencil_enabled ? &info_depth_stencil : nullptr,
};
cmd.beginRendering(info_render);
cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline);
if (_desc_sets.size() > 0) {
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipeline_layout, 0, _desc_sets, {});
}
cmd.draw(3, 1, 0, 0);
cmd.endRendering();
}
void Graphics::execute(vk::CommandBuffer cmd, Image& color_dst, vk::AttachmentLoadOp color_load) {
	vk::RenderingAttachmentInfo info_color_attach {
		.imageView = color_dst._view,
		.imageLayout = color_dst._last_layout,
		.resolveMode = 	vk::ResolveModeFlagBits::eNone,
		.loadOp = color_load,
		.storeOp = vk::AttachmentStoreOp::eStore,
		.clearValue { .color { std::array<float, 4>{ 0, 0, 0, 0 } } }
	};
	vk::RenderingInfo info_render {
		.renderArea = _render_area,
		.layerCount = 1,
		.colorAttachmentCount = 1,
		.pColorAttachments = &info_color_attach,
		.pDepthAttachment = nullptr,
		.pStencilAttachment = nullptr,
	};
	cmd.beginRendering(info_render);
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline);
	if (_desc_sets.size() > 0) {
		cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipeline_layout, 0, _desc_sets, {});
	}
	cmd.draw(3, 1, 0, 0);
	cmd.endRendering();
}

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
auto PipelineBase::reflect(vk::Device device, const vk::ArrayProxy<std::string_view>& shader_paths, const SamplerInfos& sampler_infos)
-> std::pair< vk::VertexInputBindingDescription, std::vector<vk::VertexInputAttributeDescription>> {
	// create shader reflections
	auto reflections = get_reflections(shader_paths);

	// get vertex attributes from vertex shader stage
	auto [vertex_input_desc, attr_descs] = get_vertex_desc(reflections);

	// stop when there are no bindings
	uint32_t binding_count = 0;
	for (auto& reflection: reflections) binding_count += get_reflected_bindings(reflection).size();
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