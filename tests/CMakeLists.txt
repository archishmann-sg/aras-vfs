add_subdirectory(vfs)

add_executable(${CMAKE_PROJECT_NAME}_tests)

target_sources(${CMAKE_PROJECT_NAME}_tests
    PRIVATE

    FILE_SET vfs_tests TYPE CXX_MODULES
    FILES ${VFS_TESTS}
)

find_package(Catch2 CONFIG REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME}_tests PRIVATE ${PROJECT_NAME}_static_lib stdc++exp Catch2::Catch2 Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(${CMAKE_PROJECT_NAME}_tests)
