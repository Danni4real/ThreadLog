project(test)

add_definitions("-g")

aux_source_directory(. SRC_LIST)

add_executable(${PROJECT_NAME} ${SRC_LIST})

TARGET_LINK_LIBRARIES(${PROJECT_NAME} pthread)

