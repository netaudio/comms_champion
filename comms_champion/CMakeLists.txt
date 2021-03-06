find_package (Doxygen)
if (DOXYGEN_FOUND)
    set (doc_output_dir "${DOC_INSTALL_DIR}/comms_champion")
    make_directory (${doc_output_dir})
    
    set (match_str "OUTPUT_DIRECTORY[^\n]*")
    set (replacement_str "OUTPUT_DIRECTORY = ${doc_output_dir}")
    set (output_file "${CMAKE_CURRENT_BINARY_DIR}/doxygen.conf")

    set (config_file "${CMAKE_CURRENT_SOURCE_DIR}/doc/doxygen.conf")
    file (READ ${config_file} config_text)
    string (REGEX REPLACE "${match_str}" "${replacement_str}" modified_config_text "${config_text}")
    file (WRITE "${output_file}" "${modified_config_text}")

    add_custom_target ("doc_commschampion"
            COMMAND ${DOXYGEN_EXECUTABLE} ${output_file}
            COMMAND ${CMAKE_COMMAND} -DDOC_OUTPUT_DIR="${doc_output_dir}" -P 
                            ${CMAKE_SOURCE_DIR}/cmake/DocCleanupScript.cmake
            COMMAND ${CMAKE_COMMAND} -E copy_directory 
                    ${CMAKE_CURRENT_SOURCE_DIR}/doc/image ${doc_output_dir}/html/image
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif ()

include_directories (
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/include
)

add_subdirectory (lib)
add_subdirectory (plugin)
add_subdirectory (app)
