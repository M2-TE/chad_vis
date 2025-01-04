#pragma once
#include <vulkan/vulkan.hpp>
#include <spvrc/spvrc.hpp>
#include "chad_vis/device/image.hpp"
#include "chad_vis/device/buffer.hpp"
#include "chad_vis/entities/mesh/mesh.hpp"

struct PipelineBase {
public:
	typedef std::vector<std::tuple<uint32_t /*set*/, uint32_t /*binding*/, vk::SamplerCreateInfo>> SamplerInfos; // TODO: deprecate
	void destroy(vk::Device device) {
		device.destroyPipeline(_pipeline);
		device.destroyPipelineLayout(_pipeline_layout);

		for (auto& layout: _desc_set_layouts) device.destroyDescriptorSetLayout(layout);
		for (auto& sampler: _immutable_samplers) device.destroySampler(sampler);
		device.destroyDescriptorPool(_pool);
		_desc_sets.clear();
		_desc_set_layouts.clear();
		_immutable_samplers.clear();
	}
	void write_descriptor(vk::Device device, uint32_t set, uint32_t binding, dv::Image& image, vk::DescriptorType type, vk::Sampler sampler = nullptr) {
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
		device.updateDescriptorSets(write_image, {});
	}
	void write_descriptor(vk::Device device, uint32_t set, uint32_t binding, dv::Buffer& buffer, vk::DescriptorType type, size_t offset = 0) {
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
		device.updateDescriptorSets(write_buffer, {});
	}
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
struct Compute: public PipelineBase {
	struct CreateInfo {
		vk::Device device;
		//
		std::string_view cs_path; vk::SpecializationInfo spec_info = {};
		// TODO: deprecate this one
		SamplerInfos sampler_infos = {};
	};
	void init(const CreateInfo& info) {
		// reflect shader contents
		reflect(info.device, info.cs_path, info.sampler_infos);

		// create pipeline layout
		_pipeline_layout = info.device.createPipelineLayout({
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
		if (get_module_deprecation()) cs_module = info.device.createShaderModule(info_cs);

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
		auto [result, pipeline] = info.device.createComputePipeline(nullptr, info_compute_pipe);
		if (result != vk::Result::eSuccess) std::println("error creating compute pipeline");
		_pipeline = pipeline;
		// destroy shader module if previously created
		if (get_module_deprecation()) info.device.destroyShaderModule(cs_module);
	}
	void execute(vk::CommandBuffer cmd, uint32_t nx, uint32_t ny, uint32_t nz) {
		cmd.bindPipeline(vk::PipelineBindPoint::eCompute, _pipeline);
		cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipeline_layout, 0, _desc_sets, {});
		cmd.dispatch(nx, ny, nz);
	}
};
struct Graphics: public PipelineBase {
	struct CreateInfo {
		vk::Device device;
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
	void init(const CreateInfo& info) {
		// reflect shader contents
		auto [bind_desc, attr_descs] = reflect(info.device, { info.vs_path, info.fs_path }, info.sampler_infos);

		// create pipeline layout
		vk::PipelineLayoutCreateInfo layoutInfo {
			.setLayoutCount = (uint32_t)_desc_set_layouts.size(),
			.pSetLayouts = _desc_set_layouts.data()
		};
		_pipeline_layout = info.device.createPipelineLayout(layoutInfo);

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
		if (get_module_deprecation()) vs_module = info.device.createShaderModule(info_vs);
		if (get_module_deprecation()) fs_module = info.device.createShaderModule(info_fs);
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
		auto [result, pipeline] = info.device.createGraphicsPipeline(nullptr, pipeInfo);
		if (result != vk::Result::eSuccess) std::println("error creating graphics pipeline");
		_pipeline = pipeline;
		// set persistent options
		_render_area = vk::Rect2D({ 0,0 }, info.extent);
		_depth_enabled = info.depth.test || info.depth.write;
		_stencil_enabled = info.stencil.test;
		// destroy shader modules if previously created
		if (get_module_deprecation()) info.device.destroyShaderModule(vs_module);
		if (get_module_deprecation()) info.device.destroyShaderModule(fs_module);
	}
	
	// draw fullscreen triangle with color and depth attachments
	auto execute(vk::CommandBuffer cmd,
		dv::Image& color, vk::AttachmentLoadOp color_load,
		dv::DepthStencil& depth_stencil, vk::AttachmentLoadOp depth_stencil_load)
	-> void {
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
		cmd.draw(3, 1, 0, 0);
		cmd.endRendering();
	}

	// draw fullscreen  triangle with only color attachment
	auto execute(vk::CommandBuffer cmd,
		dv::Image& color_dst, vk::AttachmentLoadOp color_load)
	-> void {
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

	// draw mesh
	template<typename Vertex, typename Index>
	auto execute(vk::CommandBuffer cmd,
		dv::Image& color, vk::AttachmentLoadOp color_load,
		dv::DepthStencil& depth_stencil, vk::AttachmentLoadOp depth_stencil_load,
		Mesh<Vertex, Index>& mesh)
	-> void {
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
	auto execute(vk::CommandBuffer cmd,
		dv::Image& color_dst, vk::AttachmentLoadOp color_load,
		Mesh<Vertex, Index>& mesh)
	-> void {
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