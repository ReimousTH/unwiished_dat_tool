set(SCRIPTS_OUTPUT_DIR $<TARGET_FILE_DIR:unwiished_dat_tool>)

set(SCRIPT_FILES
    "build_dat.bat"
    "build_dat_force.bat" 
    "padding_entries.source"
    "dat_to_json.bat"
    "json_to_dat.bat"
    "dump_dat.bat"
)

add_custom_target(scripts ALL)
target_sources(scripts PRIVATE ${SCRIPT_FILES})

# Copy each file individually
foreach(script_file ${SCRIPT_FILES})
    add_custom_command(TARGET scripts POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/${script_file}
            ${SCRIPTS_OUTPUT_DIR}
        COMMENT "Copying ${script_file}"
    )
endforeach()