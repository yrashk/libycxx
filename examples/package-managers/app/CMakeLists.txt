# A project with a C++ dependency from a package manager ({fmt}), built against libycxx with
# the dependency (docs/BUILDING_PROJECTS.md, "Package managers"; examples/package-managers).
cmake_minimum_required(VERSION 3.24)
project(ycxx_package_manager_example LANGUAGES CXX)
find_package(fmt CONFIG REQUIRED)
add_executable(app main.cpp)
target_link_libraries(app PRIVATE fmt::fmt)
