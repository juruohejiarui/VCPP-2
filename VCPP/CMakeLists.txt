cmake_minimum_required (VERSION 2.8)

project (VCPP)

set(CXX_SOURCE 
    ../Tools/tools.cpp ../Tools/message.cpp ../VASM/commandlist.cpp 
    tokenlist.cpp
    syntaxnode.cpp
    idensys.cpp
    gensymbol.cpp genvcode.cpp genhelper.cpp searchsymbol.cpp chketype.cpp syntaxcandy.cpp
    constexprcalc.cpp
    compiler.cpp
    main.cpp)
set(CXX_HEADER 
    ../Tools/tools.h ../Tools/message.h 
    ../VASM/commandlist.h 
    tokenlist.h 
    idensys.h constexprcalc.h
    gen.h geninner.h 
    compiler.h 
    syntaxnode.h)  

# set_target_properties(${TARGET} PROPERTIES
#     COMPILE_FLAGS "-pthread"
#     LINK_FLAGS "-pthread")
add_executable(VCPP ${CXX_SOURCE} ${CXX_HEADER})
find_package(Threads)
target_link_libraries(VCPP ${CMAKE_THREAD_LIBS_INIT})


set(CMAKE_CXX_COMPILER "g++")

 
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -g2 -ggdb -std=c++17")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -g -O3 -std=c++17")