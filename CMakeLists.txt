cmake_minimum_required(VERSION 3.2)
project(server)
#set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -g -Wall -Wextra -fexceptions")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall -Wextra -fexceptions")

aux_source_directory(${PROJECT_SOURCE_DIR}/src SHARED_SRCS)
add_library(common ${SHARED_SRCS})
target_include_directories(common
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)
# set the C++ standard to the appropriate standard for using auto
target_compile_features(common PRIVATE cxx_auto_type)

aux_source_directory(${PROJECT_SOURCE_DIR}/server/src SERVER_SRCS)
add_executable(server ${SERVER_SRCS})
target_include_directories(server
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/server/include
)
target_link_libraries(server
    PRIVATE
        common
        ssl
        crypto
        boost_system
        odbc
)
target_compile_features(server PRIVATE cxx_auto_type)
