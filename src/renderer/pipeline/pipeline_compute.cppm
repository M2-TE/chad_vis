module;
#include <print>
#include <cstdint>
#include <string_view>
#include <spvrc/spvrc.hpp>
export module pipeline_compute;
import pipeline_base;
import vulkan_hpp;
import device;

export struct Compute: public PipelineBase {
	struct CreateInfo;
	void init(const CreateInfo& info);
	void execute(vk::CommandBuffer cmd, uint32_t nx, uint32_t ny, uint32_t nz);
};
struct Compute::CreateInfo {
	const Device& device;
	std::string_view cs_path; vk::SpecializationInfo spec_info = {};
	SamplerInfos sampler_infos = {};
};

module: private;
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