cmake_minimum_required(VERSION 3.20)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR AND NOT CMAKE_HOST_WIN32)
    set(CMAKE_SYSTEM_NAME Windows)
    set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
    set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
    set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
endif()

project(RiftgateCompanion)
enable_language(CXX)

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_definitions(NOLOG NDEBUG WIN32_LEAN_AND_MEAN)
    add_compile_options(
        -O3
        -g0
        -DNDEBUG
        -ffunction-sections
        -fdata-sections
        -fno-ident
        -flto
    )
    add_link_options(
        -s
        -flto
        -Wl,--gc-sections
        -Wl,--strip-all
    )
endif()
if(NOLOG)
    add_compile_definitions(NOLOG)
endif()

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG 21af527e756435701f23e01aa8ea8dab6e050c90
    GIT_PROGRESS TRUE)
FetchContent_MakeAvailable(json)
FetchContent_Declare(
    minhook
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
    GIT_TAG d94c64d32ea37bc4f5ee47d580709f70c6fb6080
    GIT_PROGRESS TRUE)
FetchContent_MakeAvailable(minhook)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG a9e7a8c880cf4a7fbf142b13a41bcbb1f43d0938
    GIT_PROGRESS TRUE)
FetchContent_MakeAvailable(imgui)
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui PUBLIC
    d3d11
    d3dcompiler
    dwmapi
)
FetchContent_Declare(
    gd_dllloader
    GIT_REPOSITORY https://github.com/elmodor/grimdawn-dllloader.git
    GIT_TAG c1ff349caf55e006beefeaf563c6a5b90f5d4f75
    GIT_PROGRESS TRUE)
FetchContent_MakeAvailable(gd_dllloader)
add_custom_target(copy_winmm ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:gd_dllloader::winmm>
        $<TARGET_FILE_DIR:RiftgateCompanion>
    DEPENDS
        gd_dllloader::winmm
)

add_library(RiftgateCompanion SHARED
    dllmain.cpp
    d3d11_hook.cpp
    localization.cpp
    savesystem.cpp
    windowsfonts.cpp
)

target_link_libraries(RiftgateCompanion PRIVATE
    minhook
    imgui
    nlohmann_json::nlohmann_json # savesystem
    dwrite # windowsfonts
)

target_link_options(RiftgateCompanion PRIVATE
    -static
    -static-libgcc
    -static-libstdc++
)

set_target_properties(RiftgateCompanion PROPERTIES
    PREFIX ""
    OUTPUT_NAME "RiftgateCompanion"
)
