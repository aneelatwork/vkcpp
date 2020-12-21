project( ${CMAKE_PROJECT_NAME}_test )

add_executable( ${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/test.cpp )

target_link_libraries( ${PROJECT_NAME} PRIVATE ${CMAKE_PROJECT_NAME} )


