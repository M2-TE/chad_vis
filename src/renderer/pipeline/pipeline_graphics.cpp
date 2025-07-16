module;
#include <spvrc/spvrc.hpp>
module renderer.pipeline;

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
	// create shader modules
	vk::ShaderModule vs_module = info.device._logical.createShaderModule(info_vs);
	vk::ShaderModule fs_module = info.device._logical.createShaderModule(info_fs);
	std::array<vk::PipelineShaderStageCreateInfo, 2> shader_stages {{
		vk::PipelineShaderStageCreateInfo {
			.stage = vk::ShaderStageFlagBits::eVertex,
			.module = vs_module,
			.pName = "main",
			.pSpecializationInfo = info.vs_spec.dataSize > 0 ? &info.vs_spec : nullptr,
		},
		vk::PipelineShaderStageCreateInfo {
			.stage = vk::ShaderStageFlagBits::eFragment,
			.module = fs_module,
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
	info.device._logical.destroyShaderModule(vs_module);
	info.device._logical.destroyShaderModule(fs_module);
}
void Graphics::execute(vk::CommandBuffer cmd, Image& color, vk::AttachmentLoadOp color_load, DepthStencil& depth_stencil, vk::AttachmentLoadOp depth_stencil_load) {
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