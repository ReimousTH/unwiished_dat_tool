set(SCRIPTS_OUTPUT_DIR $<TARGET_FILE_DIR:unwiished_dat_tool>)

file(GLOB SCRIPT_FILES "*.bat" "*.dat")
add_custom_target(scripts ALL)
target_sources(scripts PRIVATE ${SCRIPT_FILES})

add_custom_command(TARGET scripts POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${SCRIPTS_OUTPUT_DIR}
    COMMENT "Copying all script files to output directory"
)