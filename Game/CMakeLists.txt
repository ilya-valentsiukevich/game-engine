add_executable(Game
        src/main.cpp
)

target_link_libraries(Game
        PRIVATE
        Engine
)


# A phony target (no OUTPUT) is always considered out of date, so this runs
# on every build invocation — unlike a POST_BUILD command on Game itself,
# which only fires when Game's own sources changed and Ninja actually
# relinks it. Asset-only edits (shaders, textures, models) need the copy
# too, since none of them touch a .cpp Game depends on.
add_custom_target(CopyAssets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Assets
        $<TARGET_FILE_DIR:Game>/Assets
        COMMENT "Copying Assets to build output directory"
)
add_dependencies(Game CopyAssets)