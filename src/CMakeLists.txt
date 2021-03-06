# add -lite external here for test project

# resources
set(GAME_RES_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/Resources")
file(GLOB_RECURSE APP_UI_RES ${GAME_RES_FOLDER}/*)

# add test cases
include(${PROJECT_SOURCE_DIR}/tests/CMakeLists.txt)

set(ENGINE_BASE_LIBS
    Core
    ScriptEngine
    GFXBase
    GFXEmpty
    GFXAgent
    GFXValidator
    FrameGraph
)

if(WIN32)
    add_definitions("-DCC_STATIC")
    add_subdirectory(win)
elseif(IOS)
	add_subdirectory(ios)
elseif(ANDROID)
    add_subdirectory(android/app/src/main/cpp)
elseif(APPLE)
    add_subdirectory(mac)

    # add_subdirectory(mac-gles)
endif()

target_compile_definitions(${APP_NAME}
    PUBLIC $<$<CONFIG:Debug>:CC_DEBUG=1>
    PRIVATE ${CC_EXTERNAL_PRIVATE_DEFINITIONS}
)
