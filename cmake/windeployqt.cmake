
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
                    COMMENT "[windeployqt] Removing build artefacts..."
  )

  add_custom_command(TARGET ${target} POST_BUILD
                    COMMAND "${CMAKE_COMMAND}" -E
                            env -- "${WINDEPLOYQT_EXECUTABLE}"
                            ${WINDEPLOYQT_ARGS}
                            --verbose 1
                            --no-compiler-runtime
                            --no-translations
                            --dir "${CMAKE_CURRENT_BINARY_DIR}/winqt/"
                            $<TARGET_FILE:${target}>
                    COMMENT "[windeployqt] Running windeployqt on target..."
  )
  install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/winqt/" DESTINATION ${SIMC_INSTALL_BIN})
endfunction()
