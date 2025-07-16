module;
#include <spvrc/spvrc.hpp>
module renderer.pipeline;

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
	// create shader module
	vk::ShaderModule cs_module =  info.device._logical.createShaderModule(info_cs);
	vk::ComputePipelineCreateInfo info_compute_pipe {
		.stage = {
			.stage = vk::ShaderStageFlagBits::eCompute,
			.module = cs_module,
			.pName = "main",
			.pSpecializationInfo = info.spec_info.dataSize > 0 ? &info.spec_info : nullptr,
		},
		.layout = _pipeline_layout,
	};
	auto [result, pipeline] = info.device._logical.createComputePipeline(nullptr, info_compute_pipe);
	if (result != vk::Result::eSuccess) std::println("error creating compute pipeline");
	_pipeline = pipeline;
	info.device._logical.destroyShaderModule(cs_module);


	// PFN_vkCreateInstance asd;
}
void Compute::execute(vk::CommandBuffer cmd, uint32_t nx, uint32_t ny, uint32_t nz) {
	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, _pipeline);
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipeline_layout, 0, _desc_sets, {});
	cmd.dispatch(nx, ny, nz);
}