/*!
* This file is a portion of Luna SDK.
* For conditions of distribution and use, see the disclaimer
* and license in LICENSE.txt
* 
* @file TextureAsset.cpp
* @author JXMaster
* @date 2020/5/9
*/
#pragma once
#include "Texture.hpp"
#include <Luna/RHI/RHI.hpp>
#include <Luna/Runtime/File.hpp>
#include <Luna/Image/Image.hpp>
#include <Luna/VFS/VFS.hpp>
#include <Luna/RHI/Utility.hpp>
#include <Luna/Image/RHIHelper.hpp>

namespace Luna
{
	struct TextureAssetUserdata
	{
		lustruct("TextureAssetUserdata", "{816CDA20-AB1C-4E24-A7CE-59E2EFE9BE1E}");

		Ref<RHI::IDescriptorSetLayout> m_mipmapping_dlayout;
		Ref<RHI::IPipelineLayout> m_mipmapping_playout;
		Ref<RHI::IPipelineState> m_mipmapping_pso;

		static constexpr u32 ENV_MAP_MIPS_COUNT = 5;

		RV init();

		RV generate_mipmaps(RHI::ITexture* resource_with_most_detailed_mip, RHI::ICommandBuffer* compute_cmdbuf);
	};
	RV TextureAssetUserdata::init()
	{
		using namespace RHI;
		lutry
		{
			{
				luset(m_mipmapping_dlayout, RHI::get_main_device()->new_descriptor_set_layout(DescriptorSetLayoutDesc({
					DescriptorSetLayoutBinding::uniform_buffer_view(0, 1, ShaderVisibilityFlag::all),
					DescriptorSetLayoutBinding::read_texture_view(TextureViewType::tex2d, 1, 1, ShaderVisibilityFlag::all),
					DescriptorSetLayoutBinding::read_write_texture_view(TextureViewType::tex2d, 2, 1, ShaderVisibilityFlag::all),
					DescriptorSetLayoutBinding::sampler(3, 1, ShaderVisibilityFlag::all)
					})));
				auto dlayout = m_mipmapping_dlayout.get();
				luset(m_mipmapping_playout, RHI::get_main_device()->new_pipeline_layout(PipelineLayoutDesc(
					{ &dlayout, 1 },
					PipelineLayoutFlag::deny_vertex_shader_access |
					
					PipelineLayoutFlag::deny_pixel_shader_access)));

				lulet(cs_blob, compile_shader("Shaders/MipmapGenerationCS.hlsl", ShaderCompiler::ShaderType::compute));
				ComputePipelineStateDesc ps_desc;
				ps_desc.pipeline_layout = m_mipmapping_playout;
				ps_desc.cs = cs_blob.cspan();
				luset(m_mipmapping_pso, RHI::get_main_device()->new_compute_pipeline_state(ps_desc));
			}
		}
		lucatchret;
		return ok;
	}
	RV TextureAssetUserdata::generate_mipmaps(RHI::ITexture* resource_with_most_detailed_mip, RHI::ICommandBuffer* compute_cmdbuf)
	{
		using namespace RHI;
		lutry
		{
			auto desc = resource_with_most_detailed_mip->get_desc();
			lucheck(desc.mip_levels);
			lucheck(desc.type == TextureType::tex2d);
			lucheck(desc.depth == 1);

			if (desc.mip_levels == 1)
			{
				return ok;
			}

			auto device = g_env->device;
			compute_cmdbuf->begin_compute_pass();
			compute_cmdbuf->set_compute_pipeline_layout(m_mipmapping_playout);
			compute_cmdbuf->set_compute_pipeline_state(m_mipmapping_pso);
			u32 cb_align = (u32)device->check_feature(DeviceFeature::uniform_buffer_data_alignment).uniform_buffer_data_alignment;
			u32 cb_size = (u32)align_upper(sizeof(Float2), cb_align);
			lulet(cb, device->new_buffer(MemoryType::upload,
				BufferDesc(BufferUsageFlag::uniform_buffer, cb_size * (desc.mip_levels - 1))));

			void* mapped = nullptr;
			luexp(cb->map(0, 0, &mapped));
			for (u32 j = 0; j < (u32)(desc.mip_levels - 1); ++j)
			{
				u32 width = max<u32>((u32)desc.width >> (j + 1), 1);
				u32 height = max<u32>(desc.height >> (j + 1), 1);
				Float2U* dst = (Float2U*)((usize)mapped + cb_size * j);
				dst->x = 1.0f / (f32)width;
				dst->y = 1.0f / (f32)height;
			}
			cb->unmap(0, USIZE_MAX);

			u32 width = desc.width / 2;
			u32 height = desc.height / 2;

			for (u32 j = 0; j < (u32)(desc.mip_levels - 1); ++j)
			{
				TextureBarrier barriers[] = {
					{resource_with_most_detailed_mip, SubresourceIndex(j, 0), TextureStateFlag::automatic, TextureStateFlag::shader_read_cs, ResourceBarrierFlag::none},
					{resource_with_most_detailed_mip, SubresourceIndex(j + 1, 0),TextureStateFlag::automatic, TextureStateFlag::shader_write_cs, ResourceBarrierFlag::none}
				};
				compute_cmdbuf->resource_barrier({}, { barriers, 2 });
				lulet(vs, device->new_descriptor_set(DescriptorSetDesc(m_mipmapping_dlayout)));
				luexp(vs->update_descriptors({
					WriteDescriptorSet::uniform_buffer_view(0, BufferViewDesc::uniform_buffer(cb, cb_size * j, cb_size)),
					WriteDescriptorSet::read_texture_view(1, TextureViewDesc::tex2d(resource_with_most_detailed_mip, Format::unknown, j, 1)),
					WriteDescriptorSet::read_write_texture_view(2, TextureViewDesc::tex2d(resource_with_most_detailed_mip, Format::unknown, j + 1, 1)),
					WriteDescriptorSet::sampler(3, SamplerDesc(Filter::linear, Filter::linear, Filter::linear, TextureAddressMode::clamp, TextureAddressMode::clamp, TextureAddressMode::clamp))
					}));
				compute_cmdbuf->set_compute_descriptor_set(0, vs);
				compute_cmdbuf->attach_device_object(vs);
				compute_cmdbuf->dispatch((u32)align_upper(width, 8) / 8, (u32)align_upper(height, 8) / 8, 1);
				width = max<u32>(width / 2, 1);
				height = max<u32>(height / 2, 1);
			}
			compute_cmdbuf->end_compute_pass();

			luexp(compute_cmdbuf->submit({}, {}, true));
			compute_cmdbuf->wait();
			luexp(compute_cmdbuf->reset());
		}
		lucatchret;
		return ok;
	}
	static R<ObjRef> load_texture_asset(object_t userdata, Asset::asset_t asset, const Path& path)
	{
		ObjRef ret;
		lutry
		{
			// Open image file.
			Path file_path = path;
			file_path.append_extension("tex");
			auto file = VFS::open_file(file_path, FileOpenFlag::read, FileCreationMode::open_existing);
			if (failed(file))
			{
				file_path.replace_extension("dds");
				file = VFS::open_file(file_path, FileOpenFlag::read, FileCreationMode::open_existing);
			}
			if (failed(file))
			{
				return file.errcode();
			}
			lulet(file_data, load_file_data(file.get()));
			if (file_data.size() >= 4 && !memcmp((const c8*)file_data.data(), "DDS ", 4))
			{
				lulet(dds_image, Image::read_dds_image(file_data.data(), file_data.size()));
				RHI::TextureDesc desc;
				switch (dds_image.desc.dimension)
				{
				case Image::DDSDimension::tex2d: desc.type = RHI::TextureType::tex2d; break;
				case Image::DDSDimension::tex3d: desc.type = RHI::TextureType::tex3d; break;
				case Image::DDSDimension::tex1d: desc.type = RHI::TextureType::tex1d; break;
				default: lupanic();
				}
				desc.format = Image::dds_to_rhi_format(dds_image.desc.format);
				if (desc.format == RHI::Format::unknown) luthrow(set_error(BasicError::not_supported(), "Unsupported DDS formats."));
				desc.width = dds_image.desc.width;
				desc.height = dds_image.desc.height;
				desc.depth = dds_image.desc.depth;
				desc.array_size = dds_image.desc.array_size;
				desc.mip_levels = dds_image.desc.mip_levels;
				desc.sample_count = 1;
				desc.usages = RHI::TextureUsageFlag::read_texture | RHI::TextureUsageFlag::read_write_texture | RHI::TextureUsageFlag::copy_source | RHI::TextureUsageFlag::copy_dest;
				if (test_flags(dds_image.desc.flags, Image::DDSFlag::texturecube))
				{
					desc.usages |= RHI::TextureUsageFlag::cube;
				}
				desc.flags = RHI::ResourceFlag::none;
				// Create resource.
				lulet(tex, g_env->device->new_texture(RHI::MemoryType::local, desc));
				// Upload data.
				lulet(upload_cmdbuf, g_env->device->new_command_buffer(g_env->async_copy_queue));
				Vector<RHI::CopyResourceData> copies;
				for (u32 item = 0; item < desc.array_size; ++item)
				{
					u32 d = desc.depth;
					for (u32 mip = 0; mip < desc.mip_levels; ++mip)
					{
						auto& subresource = dds_image.subresources[Image::calc_dds_subresoruce_index(mip, item, desc.mip_levels)];
						RHI::CopyResourceData copy = RHI::CopyResourceData::write_texture(tex, RHI::SubresourceIndex(mip, item), 0, 0, 0, dds_image.data.data() + subresource.data_offset, subresource.row_pitch, subresource.slice_pitch, subresource.width, subresource.height, d);
						copies.push_back(move(copy));
					}
					if (d > 1) d >>= 1;
				}
				luexp(copy_resource_data(upload_cmdbuf, copies.cspan()));
				tex->set_name(path.encode().c_str());
				ret = tex;
			}
			else if (file_data.size() >= 8 && !memcmp((const c8*)file_data.data(), "LUNAMIPS", 8))
			{
				u64* dp = (u64*)(file_data.data() + 8);
				u32 num_mips = (u32)*dp;
				++dp;
				Vector<Pair<u64, u64>> mip_descs;
				mip_descs.reserve(num_mips);
				for (u64 i = 0; i < num_mips; ++i)
				{
					Pair<u64, u64> p;
					p.first = dp[i * 2];
					p.second = dp[i * 2 + 1];
					mip_descs.push_back(p);
				}
				// Load texture from file.
				lulet(desc, Image::read_image_file_desc(file_data.data() + mip_descs[0].first, mip_descs[0].second));
				auto desired_format = Image::get_rhi_desired_format(desc.format);
				// Create resource.
				lulet(tex, g_env->device->new_texture(RHI::MemoryType::local, RHI::TextureDesc::tex2d(
					Image::image_to_rhi_format(desc.format), 
					RHI::TextureUsageFlag::read_texture | RHI::TextureUsageFlag::read_write_texture | RHI::TextureUsageFlag::copy_source | RHI::TextureUsageFlag::copy_dest, 
					desc.width, desc.height)));
				// Upload data
				Vector<Blob> image_data_array;
				Vector<RHI::CopyResourceData> copies;
				image_data_array.reserve(num_mips);
				copies.reserve(num_mips);
				for (u32 i = 0; i < num_mips; ++i)
				{
					Image::ImageDesc desc;
					lulet(image_data, Image::read_image_file(file_data.data() + mip_descs[i].first, mip_descs[i].second, desired_format, desc));
					copies.push_back(RHI::CopyResourceData::write_texture(tex, RHI::SubresourceIndex(i, 0), 0, 0, 0, 
						image_data.data(), pixel_size(desc.format) * desc.width, pixel_size(desc.format) * desc.width * desc.height, desc.width, desc.height, 1));
					image_data_array.push_back(move(image_data));
				}
				lulet(upload_cmdbuf, g_env->device->new_command_buffer(g_env->async_copy_queue));
				luexp(RHI::copy_resource_data(upload_cmdbuf, {copies.data(), copies.size()}));
				tex->set_name(path.encode().c_str());
				ret = tex;
			}
			else
			{
				// Load texture from file.
				lulet(desc, Image::read_image_file_desc(file_data.data(), file_data.size()));
				auto desired_format = Image::get_rhi_desired_format(desc.format);
				lulet(image_data, Image::read_image_file(file_data.data(), file_data.size(), desired_format, desc));
				// Create resource.
				lulet(tex, RHI::get_main_device()->new_texture(RHI::MemoryType::local, RHI::TextureDesc::tex2d(
					Image::image_to_rhi_format(desc.format),
					RHI::TextureUsageFlag::read_texture | RHI::TextureUsageFlag::read_write_texture | RHI::TextureUsageFlag::copy_source | RHI::TextureUsageFlag::copy_dest,
					desc.width, desc.height)));
				// Upload data.
				lulet(upload_cmdbuf, g_env->device->new_command_buffer(g_env->async_copy_queue));
				luexp(RHI::copy_resource_data(upload_cmdbuf, {RHI::CopyResourceData::write_texture(tex, RHI::SubresourceIndex(0, 0), 0, 0, 0,
					image_data.data(), pixel_size(desc.format) * desc.width, pixel_size(desc.format) * desc.width * desc.height,
					desc.width, desc.height, 1)}));
				// Generate mipmaps.
				Ref<TextureAssetUserdata> ctx = ObjRef(userdata);
				lulet(cmdbuf, g_env->device->new_command_buffer(g_env->async_compute_queue));
                cmdbuf->set_name("MipmapGeneration");
				luexp(ctx->generate_mipmaps(tex, cmdbuf));
				tex->set_name(path.encode().c_str());
				ret = tex;
			}
		}
		lucatchret;
		return ret;
	}
	RV register_static_texture_asset_type()
	{
		lutry
		{
			register_boxed_type<TextureAssetUserdata>();
			Ref<TextureAssetUserdata> userdata = new_object<TextureAssetUserdata>();
			luexp(userdata->init());
			Asset::AssetTypeDesc desc;
			desc.name = get_static_texture_asset_type();
			desc.on_load_asset = load_texture_asset;
			desc.on_save_asset = nullptr;
			desc.on_set_asset_data = nullptr;
			desc.userdata = userdata;
			Asset::register_asset_type(desc);
		}
		lucatchret;
		return ok;
	}
	Name get_static_texture_asset_type()
	{
		return "Static Texture";
	}
}
