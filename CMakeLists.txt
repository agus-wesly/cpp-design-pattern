cmake_minimum_required(VERSION 3.31.0)
project(LENIOTTechnicalTestBackend VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)

include(CTest)
enable_testing()

include_directories(
    "/opt/OpenDDS-3.29.1"
    "/opt/OpenDDS-3.29.1/ACE_wrappers"
    "/opt/OpenDDS-3.29.1/ACE_wrappers/TAO"
)
link_directories(
    "/opt/OpenDDS-3.29.1/lib"
    "/opt/OpenDDS-3.29.1/ACE_wrappers/lib"
)

file(GLOB_RECURSE SOURCES src/*.cpp src/*.cc src/*.h src/*.hpp)
list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/app.cpp)
add_library(shared-objects OBJECT ${SOURCES})

add_executable(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/src/app.cpp $<TARGET_OBJECTS:shared-objects>)

find_package(absl CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE abseil::abseil)
include_directories(${absl_INCLUDE_DIR})

find_package(gRPC CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE gRPC::gpr gRPC::grpc gRPC::grpc++ gRPC::grpc++_alts)
include_directories(${gRPC_INCLUDE_DIR})

find_package(protobuf CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE protobuf::libprotoc protobuf::libprotobuf)
include_directories(${protobuf_INCLUDE_DIR})

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tests ${CMAKE_CURRENT_BINARY_DIR}/tests)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)