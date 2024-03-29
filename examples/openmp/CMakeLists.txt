cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(omnitrace-openmp LANGUAGES CXX)

file(GLOB common_source ${CMAKE_CURRENT_SOURCE_DIR}/common/*.cpp)
add_library(openmp-common OBJECT ${common_source})
target_include_directories(openmp-common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/common)

add_executable(openmp-cg ${CMAKE_CURRENT_SOURCE_DIR}/CG/cg.cpp
                         $<TARGET_OBJECTS:openmp-common>)
add_executable(openmp-lu ${CMAKE_CURRENT_SOURCE_DIR}/LU/lu.cpp
                         $<TARGET_OBJECTS:openmp-common>)

option(USE_CLANG_OMP "Use the clang OpenMP if available" ON)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    find_package(OpenMP REQUIRED)
    target_link_libraries(openmp-common PUBLIC OpenMP::OpenMP_CXX)
    set(OMNITRACE_OPENMP_USING_LIBOMP_LIBRARY
        ON
        CACHE INTERNAL "Used by omnitrace testing" FORCE)
else()
    find_program(CLANGXX_EXECUTABLE NAMES clang++)
    find_library(
        LIBOMP_LIBRARY
        NAMES omp omp5 ${CMAKE_SHARED_LIBRARY_PREFIX}omp${CMAKE_SHARED_LIBRARY_SUFFIX}.5)
    if(CLANGXX_EXECUTABLE
       AND LIBOMP_LIBRARY
       AND COMMAND omnitrace_custom_compilation
       AND USE_CLANG_OMP)
        target_compile_options(openmp-common PUBLIC -W -Wall -fopenmp=libomp)
        target_link_libraries(openmp-common PUBLIC ${LIBOMP_LIBRARY})
        omnitrace_custom_compilation(COMPILER ${CLANGXX_EXECUTABLE} TARGET openmp-common)
        omnitrace_custom_compilation(COMPILER ${CLANGXX_EXECUTABLE} TARGET openmp-cg)
        omnitrace_custom_compilation(COMPILER ${CLANGXX_EXECUTABLE} TARGET openmp-lu)
        set(OMNITRACE_OPENMP_USING_LIBOMP_LIBRARY
            ON
            CACHE INTERNAL "Used by omnitrace testing" FORCE)
    else()
        find_package(OpenMP REQUIRED)
        target_link_libraries(openmp-common PUBLIC OpenMP::OpenMP_CXX)
        set(OMNITRACE_OPENMP_USING_LIBOMP_LIBRARY
            OFF
            CACHE INTERNAL "Used by omnitrace testing" FORCE)
    endif()
endif()

target_link_libraries(openmp-cg PRIVATE openmp-common)
target_link_libraries(openmp-lu PRIVATE openmp-common)

if(OMNITRACE_INSTALL_EXAMPLES)
    install(
        TARGETS openmp-cg openmp-lu
        DESTINATION bin
        COMPONENT omnitrace-examples)
endif()
