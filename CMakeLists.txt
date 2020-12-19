cmake_minimum_required( VERSION 3.19 )
project( vkcpp VERSION 0.1 )

include( ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake )
conan_basic_setup()

add_library( ${CMAKE_PROJECT_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/include/vkcpp/elements.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/elements.cpp
    )
target_compile_features( ${CMAKE_PROJECT_NAME} PUBLIC cxx_std_17 )

target_link_libraries( ${CMAKE_PROJECT_NAME} PUBLIC ${CONAN_LIBS} )

target_include_directories( ${CMAKE_PROJECT_NAME} 
    PUBLIC
        ${CONAN_INCLUDE_DIRS}
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

