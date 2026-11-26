cmake_minimum_required(VERSION 3.28)
project(libycxx_execution_portable LANGUAGES CXX)

find_package(Threads REQUIRED)

# Give consumers only this include tree. The full libycxx include directory would shadow
# the toolchain's standard headers, including <atomic> and <thread>.
set(_ycxx_portable_include "${CMAKE_CURRENT_BINARY_DIR}/include")
foreach(_ycxx_header IN ITEMS stop_state.inc inplace_stop_token.inc stop_token_traits.inc)
  configure_file("../include/ycxx/core/${_ycxx_header}"
    "${_ycxx_portable_include}/ycxx/core/${_ycxx_header}" COPYONLY)
endforeach()

add_library(ycxx_stop_token_portable INTERFACE)
add_library(ycxx::stop_token_portable ALIAS ycxx_stop_token_portable)
set_target_properties(ycxx_stop_token_portable PROPERTIES EXPORT_NAME stop_token_portable)
target_compile_features(ycxx_stop_token_portable INTERFACE cxx_std_20)
include(GNUInstallDirs)
set(_ycxx_install_include "${CMAKE_INSTALL_INCLUDEDIR}/libycxx-execution")
target_include_directories(ycxx_stop_token_portable SYSTEM INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<BUILD_INTERFACE:${_ycxx_portable_include}>"
  "$<INSTALL_INTERFACE:${_ycxx_install_include}>")
target_link_libraries(ycxx_stop_token_portable INTERFACE Threads::Threads)

configure_file(../src/pal/posix/thread_stack.h "${_ycxx_portable_include}/ycxx/execution/detail/thread_stack.h" COPYONLY)

find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
set(_ycxx_native "${CMAKE_CURRENT_SOURCE_DIR}/..")
file(GLOB _ycxx_exec_headers CONFIGURE_DEPENDS "${_ycxx_native}/include/ycxx/core/exec_*.hpp")
set(_ycxx_generated_headers)
foreach(_ycxx_header IN LISTS _ycxx_exec_headers)
  cmake_path(GET _ycxx_header FILENAME _ycxx_name)
  list(APPEND _ycxx_generated_headers "${_ycxx_portable_include}/ycxx/execution/detail/${_ycxx_name}")
endforeach()
set(_ycxx_scheduler
  "${CMAKE_CURRENT_BINARY_DIR}/parallel_scheduler.cpp"
  "${CMAKE_CURRENT_BINARY_DIR}/parallel_scheduler_query.cpp")
add_custom_command(
  OUTPUT ${_ycxx_scheduler} ${_ycxx_generated_headers}
  COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/project_execution.py"
    "${_ycxx_native}" "${CMAKE_CURRENT_BINARY_DIR}"
  DEPENDS project_execution.py ${_ycxx_exec_headers}
    "${_ycxx_native}/src/hosted/parallel_scheduler.cppm"
    "${_ycxx_native}/src/hosted/parallel_scheduler_query.cpp"
  VERBATIM)
add_library(ycxx_execution_portable stack_thread.cpp ${_ycxx_scheduler} ${_ycxx_generated_headers})
add_library(ycxx::execution_portable ALIAS ycxx_execution_portable)
set_target_properties(ycxx_execution_portable PROPERTIES
  EXPORT_NAME execution_portable
  CXX_SCAN_FOR_MODULES OFF
  POSITION_INDEPENDENT_CODE ON)
if(cxx_std_26 IN_LIST CMAKE_CXX_COMPILE_FEATURES)
  target_compile_features(ycxx_execution_portable PUBLIC cxx_std_26)
else()
  target_compile_options(ycxx_execution_portable PUBLIC "$<$<COMPILE_LANGUAGE:CXX>:-std=c++26>")
endif()
target_link_libraries(ycxx_execution_portable PUBLIC ycxx_stop_token_portable)


if(CMAKE_GENERATOR MATCHES "Ninja")
  add_library(ycxx_execution_portable_modules STATIC)
  add_library(ycxx::execution_portable_modules ALIAS ycxx_execution_portable_modules)
  target_sources(ycxx_execution_portable_modules PUBLIC FILE_SET CXX_MODULES
    FILES execution.cppm)
  set_target_properties(ycxx_execution_portable_modules PROPERTIES
    EXPORT_NAME execution_portable_modules POSITION_INDEPENDENT_CODE ON)
  target_compile_features(ycxx_execution_portable_modules PUBLIC cxx_std_23)
  target_link_libraries(ycxx_execution_portable_modules PUBLIC ycxx_execution_portable)
endif()


option(YCXX_EXECUTION_PORTABLE_INSTALL "Install the portable execution package" ${PROJECT_IS_TOP_LEVEL})
if(YCXX_EXECUTION_PORTABLE_INSTALL)
  include(CMakePackageConfigHelpers)
  set(_ycxx_package_dir "${CMAKE_INSTALL_LIBDIR}/cmake/libycxxExecution")
  install(FILES ../LICENSE DESTINATION "${CMAKE_INSTALL_DATADIR}/libycxx-execution")
  install(DIRECTORY include/ "${_ycxx_portable_include}/" DESTINATION "${_ycxx_install_include}")
  install(TARGETS ycxx_stop_token_portable ycxx_execution_portable EXPORT libycxxExecutionTargets)
  if(TARGET ycxx_execution_portable_modules)
    install(TARGETS ycxx_execution_portable_modules EXPORT libycxxExecutionTargets
      FILE_SET CXX_MODULES DESTINATION "${CMAKE_INSTALL_DATADIR}/libycxx-execution/modules")
  endif()
  install(EXPORT libycxxExecutionTargets NAMESPACE ycxx:: DESTINATION "${_ycxx_package_dir}"
    CXX_MODULES_DIRECTORY cxx-modules)
  configure_package_config_file(libycxxExecutionConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/libycxxExecutionConfig.cmake"
    INSTALL_DESTINATION "${_ycxx_package_dir}")
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libycxxExecutionConfig.cmake"
    DESTINATION "${_ycxx_package_dir}")
endif()
