module;
#include <vector>
#include <cstdint>
#include <cstring>
#include <string_view>
export module pipeline;
import device_buffer;
import vulkan_hpp;
import device;
import image;
import mesh;

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