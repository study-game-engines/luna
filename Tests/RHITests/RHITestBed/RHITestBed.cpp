/*!
* This file is a portion of Luna SDK.
* For conditions of distribution and use, see the disclaimer
* and license in LICENSE.txt
* 
* @file RHITestBed.cpp
* @author JXMaster
* @date 2020/8/2
*/
#include <Luna/Runtime/PlatformDefines.hpp>
#define LUNA_RHI_TESTBED_API LUNA_EXPORT
#include "RHITestBed.hpp"
#include <Luna/Runtime/Runtime.hpp>
#include <Luna/Runtime/Module.hpp>
#include <Luna/Runtime/Log.hpp>
#include <Luna/Window/Window.hpp>
#include <Luna/Runtime/Time.hpp>
#include <Luna/Runtime/Thread.hpp>
#include <Luna/RHI/Adapter.hpp>

namespace Luna
{
	namespace RHITestBed
	{
		using namespace RHI;
		using namespace Window;

		void on_window_resize(IWindow* window, u32 width, u32 height);
		void on_window_close(IWindow* window);

		RV(*m_init_func)() = nullptr;
		void(*m_close_func)() = nullptr;
		void(*m_draw_func)() = nullptr;
		void(*m_resize_func)(u32 new_width, u32 new_height) = nullptr;

		u32 m_command_queue;
		Ref<IWindow> m_window;
		Ref<ISwapChain> m_swap_chain;
		Ref<ICommandBuffer> m_command_buffer;

		Ref<ITexture> m_back_buffer;

		u64 m_time;
		u64 m_frame_count;

		LUNA_RHI_TESTBED_API void register_init_func(RV(*init_func)())
		{
			m_init_func = init_func;
		}
		LUNA_RHI_TESTBED_API void register_close_func(void(*close_func)())
		{
			m_close_func = close_func;
		}
		LUNA_RHI_TESTBED_API void register_draw_func(void(*draw_func)())
		{
			m_draw_func = draw_func;
		}
		LUNA_RHI_TESTBED_API void register_resize_func(void(*resize_func)(u32 new_width, u32 new_height))
		{
			m_resize_func = resize_func;
		}
		void on_window_resize(IWindow* window, u32 width, u32 height)
		{
			if(width && height)
			{
				// resize back buffer.
				lupanic_if_failed(m_swap_chain->reset({ width, height, 2, Format::bgra8_unorm, true }));
				if (m_resize_func) m_resize_func(width, height);
			}
		}
		void on_window_close(IWindow* window)
		{
			window->close();
		}
		RV init()
		{
			lutry
			{
				set_log_to_platform_enabled(true);
				m_command_queue = U32_MAX;
				auto device = get_main_device();
				u32 num_queues = device->get_num_command_queues();
				for (u32 i = 0; i < num_queues; ++i)
				{
					auto desc = device->get_command_queue_desc(i);
					if (desc.type == CommandQueueType::graphics && test_flags(desc.flags, CommandQueueFlag::presenting))
					{
						m_command_queue = i;
						break;
					}
				}
				if (m_command_queue == U32_MAX) return set_error(BasicError::not_supported(), "No command queue is suitable.");
				luset(m_window, new_window("RHI Test", WindowDisplaySettings::as_windowed(), WindowCreationFlag::resizable));
				m_window->get_close_event().add_handler(on_window_close);
				m_window->get_framebuffer_resize_event().add_handler(on_window_resize);
				luset(m_swap_chain, device->new_swap_chain(m_command_queue, m_window, SwapChainDesc({0, 0, 2, Format::bgra8_unorm, true})));
				auto sz = m_window->get_size();
				luset(m_command_buffer, device->new_command_buffer(m_command_queue));
				luset(m_back_buffer, m_swap_chain->get_current_back_buffer());

				auto adapters = RHI::get_adapters();
				for (u32 i = 0; i < adapters.size(); ++i)
				{
					log_info("RHITest", "Adapter %u", i);
					log_info("RHITest", "Name: %s", adapters[i]->get_name());
					log_info("RHITest", "====================");
				}
				m_time = get_ticks();
				m_frame_count = 0;
			}
			lucatchret;
			return ok;
		}
		LUNA_RHI_TESTBED_API RV run()
		{
			auto r = init();
			if (failed(r)) return r.errcode();
			if (m_init_func)
			{
				auto r = m_init_func();
				if (failed(r))
				{
					log_error("%s", explain(r.errcode()));
                    return r.errcode();
				}
			}
			while (true)
			{
				Window::poll_events();
				if (m_window->is_closed()) break;
				if (m_window->is_minimized())
				{
					sleep(100);
					continue;
				}
				++m_frame_count;
				u64 new_time = get_ticks();
				if (new_time - m_time >= get_ticks_per_second())
				{
					c8 buf[64];
					snprintf(buf, 64, "RHI Test - FPS: %lld", m_frame_count);
					lupanic_if_failed(m_window->set_title(buf));
					m_time = new_time;
					m_frame_count = 0;
				}

				lupanic_if_failed(m_command_buffer->reset());

				m_back_buffer = m_swap_chain->get_current_back_buffer().get();

				if (m_draw_func) m_draw_func();

				m_command_buffer->resource_barrier({},
					{
						{get_back_buffer(), TEXTURE_BARRIER_ALL_SUBRESOURCES, TextureStateFlag::automatic, TextureStateFlag::present, ResourceBarrierFlag::none}
					});
				lupanic_if_failed(m_command_buffer->submit({}, {}, true));
				m_command_buffer->wait();
				lupanic_if_failed(m_swap_chain->present());
			}
			if (m_close_func) m_close_func();
			m_window.reset();
			m_swap_chain.reset();
			m_back_buffer.reset();
			m_command_buffer.reset();
			return ok;
		}
		LUNA_RHI_TESTBED_API RHI::ITexture* get_back_buffer()
		{
			return m_back_buffer;
		}
		LUNA_RHI_TESTBED_API RHI::ICommandBuffer* get_command_buffer()
		{
			return m_command_buffer;
		}
		LUNA_RHI_TESTBED_API Window::IWindow* get_window()
		{
			return m_window;
		}
		LUNA_RHI_TESTBED_API u32 get_command_queue_index()
		{
			return m_command_queue;
		}
	}

	struct RHITestBedModule : public Module
	{
		virtual const c8* get_name() override { return "RHITestBed"; }
		virtual RV on_register() override
		{
			return add_dependency_modules(this, {module_rhi(), module_window()});
		}
	};
	LUNA_RHI_TESTBED_API Module* module_rhi_test_bed()
	{
		static RHITestBedModule m;
		return &m;
	}
}
