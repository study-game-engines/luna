/*!
* This file is a portion of Luna SDK.
* For conditions of distribution and use, see the disclaimer
* and license in LICENSE.txt
* 
* @file main.cpp
* @author JXMaster
* @date 2020/2/16
*/
#include "TestCommon.hpp"
#include <Luna/Runtime/Memory.hpp>
#include <Luna/Runtime/Log.hpp>
using namespace Luna;

void run()
{
	set_log_to_platform_enabled(true);
	auto handle = register_profiler_callback(memory_profiler_callback);
	array_test();
	vector_test();
	open_hash_test();
	ring_deque_test();
	string_test();
	list_test();
	robin_hood_hash_test();
	tuple_test();
	name_test();
	path_test();
	error_test();
	variant_test();
	time_test();
	file_test();
	math_test();
	serialize_test();
	invoke_test();
	function_test();
	unicode_test();
	unregister_profiler_callback(handle);
}

int main()
{
	init();
	run();
	close();
	return 0;
}