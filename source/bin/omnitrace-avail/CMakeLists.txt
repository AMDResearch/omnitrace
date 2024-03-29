# ------------------------------------------------------------------------------#
#
# omnitrace-avail target
#
# ------------------------------------------------------------------------------#

add_executable(omnitrace-avail)

target_sources(
    omnitrace-avail
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}/avail.cpp
            ${CMAKE_CURRENT_LIST_DIR}/avail.hpp
            ${CMAKE_CURRENT_LIST_DIR}/common.cpp
            ${CMAKE_CURRENT_LIST_DIR}/common.hpp
            ${CMAKE_CURRENT_LIST_DIR}/component_categories.hpp
            ${CMAKE_CURRENT_LIST_DIR}/defines.hpp
            ${CMAKE_CURRENT_LIST_DIR}/enumerated_list.hpp
            ${CMAKE_CURRENT_LIST_DIR}/generate_config.cpp
            ${CMAKE_CURRENT_LIST_DIR}/generate_config.hpp
            ${CMAKE_CURRENT_LIST_DIR}/get_availability.hpp
            ${CMAKE_CURRENT_LIST_DIR}/get_categories.hpp
            ${CMAKE_CURRENT_LIST_DIR}/info_type.cpp
            ${CMAKE_CURRENT_LIST_DIR}/info_type.hpp)

target_include_directories(omnitrace-avail PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_compile_definitions(omnitrace-avail PRIVATE OMNITRACE_EXTERN_COMPONENTS=0)
target_link_libraries(
    omnitrace-avail
    PRIVATE omnitrace::omnitrace-compile-definitions
            omnitrace::omnitrace-interface-library omnitrace::libomnitrace-static)
set_target_properties(
    omnitrace-avail PROPERTIES BUILD_RPATH "\$ORIGIN:\$ORIGIN/../${CMAKE_INSTALL_LIBDIR}"
                               INSTALL_RPATH "${OMNITRACE_EXE_INSTALL_RPATH}")

omnitrace_strip_target(omnitrace-avail)

install(
    TARGETS omnitrace-avail
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    OPTIONAL)
