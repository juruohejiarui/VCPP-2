cmake_minimum_required (VERSION 2.8)

project (VM)

set(C_SOURCE tools.c mmanage.c vm.c typedata.c main.c rflsys.c)
set(C_HEADER tools.h mmanage.h vm.h typedata.h rflsys.h)  

add_executable(VM ${C_SOURCE} ${C_HEADER})
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(VM ${CMAKE_THREAD_LIBS_INIT})
# find_package(Threads)
# target_link_libraries(vm_c ${MAKE_THREAD_LIBS_INIT})
# target_sources(vm_c PUBLIC ${CPP_SOURCE} ${CPP_HEADER})

# set_target_properties(${TARGET} PROPERTIES
#     COMPILE_FLAGS "-pthread"
#     LINK_FLAGS "-pthread")


# set the compiler
set(CMAKE_C_COMPILER "gcc")

 
# set the arguments
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_C_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -g2 -ggdb --std=c11")
set(CMAKE_C_FLAGS_RELEASE "$ENV{CXXFLAGS} -Ofast --std=c11")