# Finds target dependencies on shared libraries so they can
# be copied next to the exe.
#----------------------------------------------------------

# Finds all target dependencies
function(get_link_libraries OUTPUT_LIST _TARGET)
  get_target_property(LIBS ${_TARGET} LINK_LIBRARIES)

  list(APPEND VISITED_TARGETS ${TARGET})

  set(LIB_FILES "")
  foreach(LIB ${LIBS})
  
    if (TARGET ${LIB})
      list(FIND VISITED_TARGETS ${LIB} VISITED)
      if (${VISITED} EQUAL -1)
        get_link_libraries(LINK_LIBS ${LIB})
        list(APPEND LIB_FILES ${LIB} ${LINK_LIBS})
      endif()
    endif()
  endforeach()
  set(VISITED_TARGETS ${VISITED_TARGETS} PARENT_SCOPE)
  set(${OUTPUT_LIST} ${LIB_FILES} PARENT_SCOPE)
endfunction()

# Copy dependent shared libraries next to executable target
function(target_copy_shared_libraries _TARGET)
  get_link_libraries(LINKED_TARGETS ${_TARGET})

  foreach(LINKED_TARGET ${LINKED_TARGETS})
    get_target_property(TARGET_TYPE ${LINKED_TARGET} TYPE)
    if(TARGET_TYPE STREQUAL "SHARED_LIBRARY")
      list(APPEND DLL_TARGETS "$<TARGET_FILE:${LINKED_TARGET}>")
    endif()
  endforeach()

  if(DLL_TARGETS)
    add_custom_command(
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${_TARGET}_dll_copy"
      DEPENDS ${DLL_TARGETS}
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DLL_TARGETS} "./"
      COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}/${_TARGET}_dll_copy"
      VERBATIM)

      # This allows to create a dependency with the command above, so command is executed again when target is built AND a DLL changed
    target_sources(${_TARGET} PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/${_TARGET}_dll_copy")
  endif()
endfunction()