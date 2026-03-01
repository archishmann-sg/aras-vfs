add_subdirectory(core)

add_library(${PROJECT_NAME}_static_lib STATIC)

target_sources(${PROJECT_NAME}_static_lib
    PUBLIC

    FILE_SET core TYPE CXX_MODULES
    FILES ${CORE_MODULES}
)

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME}
    PRIVATE
    main.cpp
)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_static_lib)
