set(RESOURCE_HEADERS "")

# Embed a given resource into a header file.
function(embed_resource input output)
  cmake_path(ABSOLUTE_PATH input NORMALIZE)
  cmake_path(ABSOLUTE_PATH output NORMALIZE)
  if(NOT EXISTS ${input})
    message(FATAL_ERROR "Resource file '${input}' does not exist.")
  endif()

  if(NOT "${output}" IN_LIST "${RESOURCE_HEADERS}")
    file(WRITE ${output} "")
    set(RESOURCE_HEADERS ${RESOURCE_HEADERS} ${output} PARENT_SCOPE)
  endif()

  # Get basename
  cmake_path(GET input FILENAME c_name)
  # Replace chars for C variable compatibility
  string(REGEX REPLACE "\\.| |-" "_" c_name ${c_name})

  # Read data as hex
  file(READ ${input} hexdata HEX)
  # Convert to C array format
  string(REGEX REPLACE "[0-9a-f][0-9a-f]" "0x\\0," hexdata ${hexdata})
  # Append data and size to file
  file(APPEND ${output} "const unsigned char ${c_name}_DATA[] = {${hexdata}};\n")
  file(APPEND ${output} "const size_t ${c_name}_SIZE = sizeof(${c_name}_DATA);\n")

endfunction()


# Copy the given DLLs (or anything, really) into the build dir
function(copy_dlls)
  cmake_parse_arguments(PARSE_ARGV 0 COPY_DLLS "" "TARGET" "DLLS" )
  add_custom_command(
    TARGET ${COPY_DLLS_TARGET}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different 
      ${COPY_DLLS_DLLS} 
      $<TARGET_FILE_DIR:${COPY_DLLS_TARGET}>
  )
endfunction()
