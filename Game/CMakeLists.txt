add_executable(Game
        src/main.cpp
)

target_link_libraries(Game
        PRIVATE
        Engine
)

add_custom_command(TARGET Game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Assets
        $<TARGET_FILE_DIR:Game>/Assets
)