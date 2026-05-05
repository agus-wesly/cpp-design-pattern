cmake_minimum_required(VERSION 3.31.0)
project(LENIOTTechnicalTestBackendTest VERSION 0.1.0 LANGUAGES C CXX)

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

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cc ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.h ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.hpp)
add_library(objects OBJECT ${SOURCES})
add_executable(${PROJECT_NAME} $<TARGET_OBJECTS:objects> $<TARGET_OBJECTS:shared-objects>)

target_link_libraries(${PROJECT_NAME} PRIVATE abseil::abseil)
target_link_libraries(${PROJECT_NAME} PRIVATE gRPC::gpr gRPC::grpc gRPC::grpc++ gRPC::grpc++_alts)
target_link_libraries(${PROJECT_NAME} PRIVATE protobuf::libprotoc protobuf::libprotobuf)

find_package(GTest CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE gtest::gtest)
include_directories(${GTest_INCLUDE_DIRS})

add_test(${PROJECT_NAME} ${PROJECT_NAME})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
