set (SRC_ROOT_PATH ${PROJECT_ROOT_PATH}/src)
set (SRCS)
list (APPEND SRCS ${SRC_ROOT_PATH}/hornet.c)
list (APPEND SRCS ${SRC_ROOT_PATH}/socket.c)
list (APPEND SRCS ${SRC_ROOT_PATH}/time.c)

set (INCS)
list (APPEND INCS ${PROJECT_ROOT_PATH}/inc)
include_directories (${INCS})

add_library (${PROJECT_NAME}-Shared SHARED ${SRCS})
set_target_properties (${PROJECT_NAME}-Shared PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
add_library (${PROJECT_NAME}-Static STATIC ${SRCS})
set_target_properties (${PROJECT_NAME}-Static PROPERTIES OUTPUT_NAME ${PROJECT_NAME})

if (DEFINED TOOLS_ROOT_PATH)
  add_executable (${PROJECT_NAME}EchoServer ${TOOLS_ROOT_PATH}/hornet_echo_server.cpp)
  add_executable (${PROJECT_NAME}EchoClient ${TOOLS_ROOT_PATH}/hornet_echo_client.cpp)
endif ()