luna_sdk_module_target("ObjLoader")
    add_headerfiles("*.hpp", {prefixdir = "Luna/ObjLoader"})
    add_files("Source/**.cpp")
    add_luna_modules("Runtime")
target_end()