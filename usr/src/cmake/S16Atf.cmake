set(CMAKE_INSTALL_TESTDIR ${CMAKE_INSTALL_PREFIX}/tests)

function(installTopKyuafile relpath)
  install(
    FILES "${CMAKE_SOURCE_DIR}/../tests/Kyuafile"
    DESTINATION ${CMAKE_INSTALL_TESTDIR}/${relpath}
  )
endfunction(installTopKyuafile)

function(addTests)
  # This works because we don't set Project() in the tests CMakeLists.txt
  string(REPLACE ${CMAKE_SOURCE_DIR} "" CMAKE_RELATIVE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
  set(t_installdir "${CMAKE_INSTALL_TESTDIR}/${CMAKE_RELATIVE_SOURCE_DIR}")
  set(t_kyuafile "${PROJECT_SOURCE_DIR}/tests/Kyuafile")

  install(
    FILES "${t_kyuafile}"
    DESTINATION ${t_installdir}
  )

  if (ARGN)
    install(
      TARGETS ${ARGV}
      DESTINATION ${t_installdir}
    )
  endif()

  foreach(test IN ITEMS ${ARGV})
    add_test(
      NAME ${test}
      COMMAND kyua test --kyuafile ${PROJECT_SOURCE_DIR}/tests/Kyuafile
        --build-root ${PROJECT_BINARY_DIR} ${test}
    )
  endforeach()
endfunction(addTests)

function(addTest name libs)
  add_executable (${name} tests/${name}.c)
  target_include_directories(${name} PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries (${name} ${libs} Atf-C::Atf-C)
  set(s16_test_list ${s16_test_list} ${name} PARENT_SCOPE)
endfunction()