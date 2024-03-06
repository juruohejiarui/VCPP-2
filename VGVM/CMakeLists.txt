cmake_minimum_required (VERSION 2.8)

project (VGVM)

set(C_SOURCE tmpl.c gtmpl.c instruction.c ../VM/mmanage.c ../VM/rflsys.c tools.c ../VM/tools.c vgvm.c syscall.c main.c)
set(C_HEADER tmpl.h gtmpl.h insttmpl.h instruction.h ../VM/mmanage.h ../VM/rflsys.h tools.h ../VM/tools.h syscall.h vgvm.h)  

add_executable(VGVM ${C_SOURCE} ${C_HEADER})
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(VGVM ${CMAKE_THREAD_LIBS_INIT})
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
set(LINK_FLAGS "")
set(CMAKE_C_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -g2 -ggdb -DNDEBUG -std=gnu99")
set(CMAKE_C_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -std=gnu99 -DNDEBUG")