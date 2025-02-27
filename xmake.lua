
target("radfoam-vulkan-viewer")
    set_languages("c++20")
    set_kind("binary")
    set_rundir("$(projectdir)")

    -- before_build(function (target)
    --     os.run("src\\compile.bat")
    -- end)

    add_includedirs("./lib/glm/glm")

    add_includedirs("C:/Users/Lozical/scoop/apps/glfw/current/include")
    add_linkdirs("C:/Users/Lozical/scoop/apps/glfw/current/lib-mingw-w64") 
    add_links("glfw3")

    add_includedirs("C:/VulkanSDK/1.3.296.0/Include") 
    add_linkdirs("C:/VulkanSDK/1.3.296.0/Lib") 
    add_links("vulkan-1")
    
    add_syslinks("gdi32", "user32", "shell32")

    
    add_files("main.cpp")
    add_files("src/*.cpp")