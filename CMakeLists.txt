cmake_minimum_required(VERSION 3.13)
project(app)

add_subdirectory(third_party)

set(APP_HEADERS
    include/boilerplate/application_base.h
    include/boilerplate/application.h
    include/boilerplate/pvm_volume.h
)

set(APP_SRC
    src/application_base.cpp
    src/application.cpp
    src/main.cpp
    src/pvm_volume.cpp
    src/webgpu.cpp
)

add_executable(app ${APP_HEADERS} ${APP_SRC})
set_target_properties(app PROPERTIES CXX_STANDARD 20)
target_link_libraries(app PRIVATE glfw glfw3webgpu glm imgui volumeio webgpu)
target_copy_webgpu_binaries(app)

target_include_directories(app PRIVATE include)

if (MSVC)
    target_compile_options(app PRIVATE /W4)
else()
    target_compile_options(app PRIVATE -Wall -Wextra -pedantic)
endif()

if(XCODE)
    set_target_properties(app PROPERTIES
        XCODE_GENERATE_SCHEME ON
        XCODE_SCHEME_ENABLE_GPU_FRAME_CAPTURE_MODE "Metal")
endif()