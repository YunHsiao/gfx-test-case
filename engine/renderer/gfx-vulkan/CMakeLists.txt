set(TARGET_NAME GFXVulkan)

set(CURRENT_DIR ${COCOS_SRC_PATH}/renderer/gfx-vulkan)

file(GLOB_RECURSE HEADER_FILES ${CURRENT_DIR}/*.h)
file(GLOB_RECURSE SOURCE_CPP_FILES ${CURRENT_DIR}/*.cpp ${CURRENT_DIR}/*.cc)
file(GLOB_RECURSE SOURCE_C_FILES ${CURRENT_DIR}/*.c)

if(COCOS_PLATFORM_IOS OR COCOS_PLATFORM_OSX)
	file(GLOB_RECURSE SOURCE_MM_FILES ${CURRENT_DIR}/*.m ${CURRENT_DIR}/*.mm)
endif()

set(ALL_FILES
  ${HEADER_FILES}
  ${SOURCE_CPP_FILES}
  ${SOURCE_C_FILES}
  )

if(COCOS_PLATFORM_IOS OR COCOS_PLATFORM_OSX)
	set(ALL_FILES ${ALL_FILES} ${SOURCE_MM_FILES})
endif()

add_definitions("-DVK_NO_PROTOTYPES")

if(WIN32)
	add_definitions("-DVK_USE_PLATFORM_WIN32_KHR")
	add_definitions("-DNOMINMAX")
elseif(COCOS_PLATFORM_IOS OR COCOS_PLATFORM_OSX)
	add_definitions("-DVK_USE_PLATFORM_METAL_EXT")
elseif(COCOS_PLATFORM_ANDROID)
	add_definitions("-DVK_USE_PLATFORM_ANDROID_KHR")
else()
	add_definitions("-DVK_USE_PLATFORM_XLIB_KHR")
endif()

add_definitions("-DBARRIER_DEDUCTION_LEVEL=0")

add_definitions("-DCC_VK_EXPORTS")
if(WIN32)
	add_definitions("-DGLEW_BUILD")
endif()
if(COCOS_UNICODE)
	add_definitions("-DUNICODE -D_UNICODE")
endif()

if (COCOS_STATIC)
	add_library(${TARGET_NAME} STATIC ${HEADER_FILES} ${ALL_FILES})
else()
	add_library(${TARGET_NAME} SHARED ${HEADER_FILES} ${ALL_FILES})
endif()

target_link_libraries(${TARGET_NAME} Core GFXBase glslang OGLCompiler OSDependent SPIRV glslang-default-resource-limits tbb tbbmalloc tbbmalloc_proxy)
target_compile_definitions(${TARGET_NAME} PRIVATE ${CC_EXTERNAL_PRIVATE_DEFINITIONS})

set_target_properties(${TARGET_NAME} PROPERTIES OUTPUT_NAME CCGFXVulkan)
set_target_properties(${TARGET_NAME} PROPERTIES OUTPUT_NAME_DEBUG CCGFXVulkan_d)

use_precompiled_header("${CMAKE_CURRENT_SOURCE_DIR}/VKStd.h" "${CMAKE_CURRENT_SOURCE_DIR}/VKStd.cpp" SOURCE_CPP_FILES)

message(STATUS "${TARGET_NAME} configuration completed.")

target_compile_definitions(${TARGET_NAME} PUBLIC
    $<$<CONFIG:Debug>:CC_DEBUG=1>
)
