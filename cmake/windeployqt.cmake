
function(windeployqt target)
  find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}" REQUIRED)

  # Bundle Library Files
  if(CMAKE_BUILD_TYPE_UPPER STREQUAL "DEBUG")
      set(WINDEPLOYQT_ARGS --debug)
  else()
      set(WINDEPLOYQT_ARGS --release)
  endif()

  message(VERBOSE "WINDEPLOYQT_EXECUTABLE ${WINDEPLOYQT_EXECUTABLE}")

  add_custom_command(TARGET ${target} POST_BUILD
                    COMMAND "${CMAKE_COMMAND}" -E remove_directory "${CMAKE_CURRENT_BINARY_DIR}/winqt/"
                    COMMAND "${CMAKE_COMMAND}" -E
                            env PATH="${_qt_bin_dir}" "${WINDEPLOYQT_EXECUTABLE}"
                            ${WINDEPLOYQT_ARGS}
                            --verbose 0
                            --no-compiler-runtime
                            --no-translations
                            --dir "${CMAKE_CURRENT_BINARY_DIR}/winqt/"
                            $<TARGET_FILE:${target}>
                    COMMENT "Deploying Qt..."
  )
  install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/winqt/" DESTINATION ${SIMC_INSTALL_BIN})
endfunction()
