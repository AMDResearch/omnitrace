#
set(component_sources
    ${CMAKE_CURRENT_LIST_DIR}/backtrace.cpp
    ${CMAKE_CURRENT_LIST_DIR}/backtrace_metrics.cpp
    ${CMAKE_CURRENT_LIST_DIR}/backtrace_timestamp.cpp
    ${CMAKE_CURRENT_LIST_DIR}/callchain.cpp
    ${CMAKE_CURRENT_LIST_DIR}/comm_data.cpp
    ${CMAKE_CURRENT_LIST_DIR}/cpu_freq.cpp
    ${CMAKE_CURRENT_LIST_DIR}/exit_gotcha.cpp
    ${CMAKE_CURRENT_LIST_DIR}/fork_gotcha.cpp
    ${CMAKE_CURRENT_LIST_DIR}/mpi_gotcha.cpp
    ${CMAKE_CURRENT_LIST_DIR}/numa_gotcha.cpp
    ${CMAKE_CURRENT_LIST_DIR}/pthread_gotcha.cpp
    ${CMAKE_CURRENT_LIST_DIR}/pthread_create_gotcha.cpp
    ${CMAKE_CURRENT_LIST_DIR}/pthread_mutex_gotcha.cpp)

set(component_headers
    ${CMAKE_CURRENT_LIST_DIR}/backtrace.hpp
    ${CMAKE_CURRENT_LIST_DIR}/backtrace_metrics.hpp
    ${CMAKE_CURRENT_LIST_DIR}/backtrace_timestamp.hpp
    ${CMAKE_CURRENT_LIST_DIR}/callchain.hpp
    ${CMAKE_CURRENT_LIST_DIR}/category_region.hpp
    ${CMAKE_CURRENT_LIST_DIR}/comm_data.hpp
    ${CMAKE_CURRENT_LIST_DIR}/cpu_freq.hpp
    ${CMAKE_CURRENT_LIST_DIR}/ensure_storage.hpp
    ${CMAKE_CURRENT_LIST_DIR}/exit_gotcha.hpp
    ${CMAKE_CURRENT_LIST_DIR}/fork_gotcha.hpp
    ${CMAKE_CURRENT_LIST_DIR}/mpi_gotcha.hpp
    ${CMAKE_CURRENT_LIST_DIR}/numa_gotcha.hpp
    ${CMAKE_CURRENT_LIST_DIR}/rcclp.hpp
    ${CMAKE_CURRENT_LIST_DIR}/rocprofiler.hpp
    ${CMAKE_CURRENT_LIST_DIR}/roctracer.hpp
    ${CMAKE_CURRENT_LIST_DIR}/pthread_gotcha.hpp
    ${CMAKE_CURRENT_LIST_DIR}/pthread_create_gotcha.hpp
    ${CMAKE_CURRENT_LIST_DIR}/pthread_mutex_gotcha.hpp)

target_sources(omnitrace-object-library PRIVATE ${component_sources} ${component_headers})

if(OMNITRACE_USE_ROCPROFILER)
    target_sources(omnitrace-object-library
                   PRIVATE ${CMAKE_CURRENT_LIST_DIR}/rocprofiler.cpp)
endif()

if(OMNITRACE_USE_ROCTRACER)
    target_sources(omnitrace-object-library
                   PRIVATE ${CMAKE_CURRENT_LIST_DIR}/roctracer.cpp)
endif()

if(OMNITRACE_USE_RCCL)
    target_sources(omnitrace-object-library PRIVATE ${CMAKE_CURRENT_LIST_DIR}/rcclp.cpp)
endif()
