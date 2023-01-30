/*!
* This file is a portion of Luna SDK.
* For conditions of distribution and use, see the disclaimer
* and license in LICENSE.txt
* 
* @file AssetBrowser.hpp
* @author JXMaster
* @date 2020/4/29
*/
#pragma once
#include "StudioHeader.hpp"
#include <Runtime/HashSet.hpp>

namespace Luna
{
	struct MainEditor;

	//! Asset browser context.
	struct AssetBrowser
	{
		lustruct("AssetBrowser", "{D38682E6-CE9C-4317-870F-40944D82281F}");

		MainEditor* m_editor;

		//u32 m_index;	// The index of the asset browser.
		//bool m_open;	// If the browser is closed.

		//----- Begin of States for navbar -----
		Vector<Path> m_histroy_paths;
		Path m_path;
		String m_path_edit_text;
		u32 m_current_location_in_histroy_path;
		bool m_is_text_editing;
		//----- End of States for navbar -----

		//----- Begin of States for tile context -----
		f32 m_tile_size;
		//Float2U m_ms_mouse_begin_pos;	// ms for multi select.
		//bool m_ms_is_dragging;
		HashSet<Name> m_selections;
		//----- End of States for tile context -----

		AssetBrowser() :
			m_current_location_in_histroy_path(0),
			m_is_text_editing(false),
			m_tile_size(128) {}
			//m_ms_is_dragging(false) {}

		void change_path(const Path& path);

		void render();

	private:
		void navbar();
		void tile_context();
	};
}