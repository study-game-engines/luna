/*!
* This file is a portion of Luna SDK.
* For conditions of distribution and use, see the disclaimer
* and license in LICENSE.txt
* 
* @file RHITestBed.hpp
* @author JXMaster
* @date 2020/8/2
*/
#pragma once

#include <Luna/RHI/RHI.hpp>
#include <Luna/ShaderCompiler/ShaderCompiler.hpp>

#ifndef LUNA_RHI_TESTBED_API
#define LUNA_RHI_TESTBED_API
#endif

namespace Luna
{
	namespace RHITestBed
	{
		LUNA_RHI_TESTBED_API void register_init_func(RV(*init_func)());
		LUNA_RHI_TESTBED_API void register_close_func(void(*close_func)());
		LUNA_RHI_TESTBED_API void register_draw_func(void(*draw_func)());
		LUNA_RHI_TESTBED_API void register_resize_func(void(*resize_func)(u32 new_width, u32 new_height));

		LUNA_RHI_TESTBED_API RV run();
		LUNA_RHI_TESTBED_API RHI::ITexture* get_back_buffer();
		LUNA_RHI_TESTBED_API RHI::ICommandBuffer* get_command_buffer();
		LUNA_RHI_TESTBED_API Window::IWindow* get_window();
		LUNA_RHI_TESTBED_API u32 get_command_queue_index();
	}

	struct Module;
	LUNA_RHI_TESTBED_API Module* module_rhi_test_bed();
}