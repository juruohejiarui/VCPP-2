cmake_minimum_required (VERSION 2.8)

project (VASM)

set(CXX_SOURCE ../Tools/tools.cpp ../Tools/message.cpp commandlist.cpp vobjbuilder.cpp main.cpp)
set(CXX_HEADER ../Tools/tools.h ../Tools/message.h commandlist.h vobjbuilder.h)  

# set_target_properties(${TARGET} PROPERTIES
#     COMPILE_FLAGS "-pthread"
#     LINK_FLAGS "-pthread")
add_executable(VASM ${CXX_SOURCE} ${CXX_HEADER})
find_package(Threads)
target_link_libraries(VASM ${CMAKE_THREAD_LIBS_INIT})


# ��ʽָ��������
if (APPLE)
    set(CMAKE_CXX_COMPILER "g++-11")
else()
    set(CMAKE_CXX_COMPILER "g++")
endif()
 
# ����������Ϣ
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -g2 -ggdb -std=c++14")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -DNDEBUG -O0 -std=c++14")