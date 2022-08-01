#!/bin/bash -e

SCRIPT_DIR=$(realpath $(dirname ${BASH_SOURCE[0]}))
cd $(dirname ${SCRIPT_DIR})

tolower()
{
    echo "$@" | awk -F '\|~\|' '{print tolower($1)}';
}

toupper()
{
    echo "$@" | awk -F '\|~\|' '{print toupper($1)}';
}

: ${CMAKE_BUILD_PARALLEL_LEVEL:=$(nproc)}
: ${DASHBOARD_MODE:="Continuous"}
: ${DASHBOARD_STAGES:="Start Update Configure Build Test MemCheck Coverage Submit"}
: ${SOURCE_DIR:=${PWD}}
: ${BINARY_DIR:=${PWD}/build}
: ${SITE:=$(hostname)}
: ${NAME:=""}
: ${SUBMIT_URL:=""}

usage()
{
    print_option() { printf "    --%-20s %-24s     %s\n" "${1}" "${2}" "${3}"; }
    echo "Options:"
    print_option "help -h" "" "This message"

    echo ""
    print_default_option() { printf "    --%-20s %-24s     %s (default: %s)\n" "${1}" "${2}" "${3}" "$(tolower ${4})"; }
    print_default_option "name -n" "<NAME>" "Job name" ""
    print_default_option "site -s" "<NAME>" "Site name" "${SITE}"
    print_default_option "source-dir -S" "<N>" "Source directory" "${SOURCE_DIR}"
    print_default_option "binary-dir -B" "<N>" "Build directory" "${BINARY_DIR}"
    print_default_option "build-jobs -j" "<N>" "Number of build jobs" "${CMAKE_BUILD_PARALLEL_LEVEL}"
    print_default_option cmake-args "<ARGS...>" "CMake configuration args" "none"
    print_default_option cdash-mode "<ARGS...>" "CDash mode" "${DASHBOARD_MODE}"
    print_default_option cdash-stages "<ARGS...>" "CDash stages" "${DASHBOARD_STAGES}"
    print_default_option submit-url "<URL>" "CDash submission URL" "${SUBMIT_URL}"
    #print_default_option lto "[on|off]" "Enable LTO" "${LTO}"
}

send-error()
{
    usage
    echo -e "\nError: ${@}"
    exit 1
}

verbose-run()
{
    echo -e "\n### Executing \"${@}\"... ###\n"
    eval $@
}

reset-last()
{
    last() { send-error "Unsupported argument :: ${1}"; }
}

reset-last

n=0
while [[ $# -gt 0 ]]
do
    case "${1}" in
        -h|--help)
            usage
            exit 0
            ;;
        -n|--name)
            shift
            NAME=${1}
            reset-last
            ;;
        -s|--site)
            shift
            SITE=${1}
            reset-last
            ;;
        -S|--source-dir)
            shift
            SOURCE_DIR=${1}
            reset-last
            ;;
        -B|--binary-dir)
            shift
            BINARY_DIR=${1}
            reset-last
            ;;
        -j|--build-jobs)
            shift
            CMAKE_BUILD_PARALLEL_LEVEL=${1}
            reset-last
            ;;
        "--cmake-args")
            shift
            CMAKE_ARGS=${1}
            last() { CMAKE_ARGS="${CMAKE_ARGS} ${1}"; }
            ;;
        "--cdash-mode")
            shift
            DASHBOARD_MODE=${1}
            reset-last
            ;;
        "--cdash-stages")
            shift
            DASHBOARD_STAGES=${1}
            last() { DASHBOARD_STAGES="${DASHBOARD_STAGES} ${1}"; }
            ;;
        "--submit-url")
            shift
            SUBMIT_URL=${1}
            reset-last
            ;;
        "--*")
            send-error "Unsupported argument at position $((${n} + 1)) :: ${1}"
            ;;
        *)
            last ${1}
            ;;
    esac
    n=$((${n} + 1))
    shift
done

if [ -z "${NAME}" ]; then send-error "--name option required"; fi

CDASH_ARGS=""
for i in ${DASHBOARD_STAGES}
do
    if [ -z "${CDASH_ARGS}" ]; then
        CDASH_ARGS="-D ${DASHBOARD_MODE}${i}"
    else
        CDASH_ARGS="${CDASH_ARGS} -D ${DASHBOARD_MODE}${i}"
    fi
done

export CMAKE_BUILD_PARALLEL_LEVEL

GIT_CMD=$(which git)
CMAKE_CMD=$(which cmake)
CTEST_CMD=$(which ctest)
SOURCE_DIR=$(realpath ${SOURCE_DIR})
BINARY_DIR=$(realpath ${BINARY_DIR})

verbose-run mkdir -p ${BINARY_DIR}

cat << EOF > ${BINARY_DIR}/CTestCustom.cmake

set(CTEST_PROJECT_NAME "Omnitrace")
set(CTEST_NIGHTLY_START_TIME "01:00:00 UTC")

set(CTEST_DROP_METHOD "http")
set(CTEST_DROP_SITE_CDASH TRUE)
set(CTEST_SUBMIT_URL "https://${SUBMIT_URL}")

set(CTEST_UPDATE_TYPE git)
set(CTEST_UPDATE_VERSION_ONLY TRUE)
set(CTEST_GIT_INIT_SUBMODULES TRUE)

set(CTEST_OUTPUT_ON_FAILURE TRUE)
set(CTEST_USE_LAUNCHERS TRUE)
set(CMAKE_CTEST_ARGUMENTS "--verbose")

set(CTEST_CUSTOM_MAXIMUM_NUMBER_OF_ERRORS "100")
set(CTEST_CUSTOM_MAXIMUM_NUMBER_OF_WARNINGS "100")
set(CTEST_CUSTOM_MAXIMUM_PASSED_TEST_OUTPUT_SIZE "10485760")

set(CTEST_SITE "${SITE}")
set(CTEST_BUILD_NAME "${NAME}")

set(CTEST_SOURCE_DIRECTORY ${SOURCE_DIR})
set(CTEST_BINARY_DIRECTORY ${BINARY_DIR})

set(CTEST_COMMAND ${CTEST_CMD})
set(CTEST_UPDATE_COMMAND ${GIT_CMD})
set(CTEST_CONFIGURE_COMMAND "${CMAKE_CMD} -B ${BINARY_DIR} ${SOURCE_DIR} -DOMNITRACE_BUILD_CI=ON ${CMAKE_ARGS}")
set(CTEST_BUILD_COMMAND "${CMAKE_CMD} --build ${BINARY_DIR} --target all")
EOF

verbose-run cd ${BINARY_DIR}

cat << EOF > dashboard.cmake

include("\${CMAKE_CURRENT_LIST_DIR}/CTestCustom.cmake")

macro(handle_error _message _ret)
    if(NOT \${\${_ret}} EQUAL 0)
        ctest_submit(PARTS Done RETURN_VALUE _submit_ret)
        message(FATAL_ERROR "\${_message} failed: \${\${_ret}}")
    endif()
endmacro()

ctest_start(${DASHBOARD_MODE})
ctest_update(SOURCE "${SOURCE_DIR}")
ctest_submit(PARTS Start Update RETURN_VALUE _submit_ret)

ctest_configure(BUILD "${BINARY_DIR}" RETURN_VALUE _configure_ret)
ctest_submit(PARTS Configure RETURN_VALUE _submit_ret)

handle_error("Configure" _configure_ret)

ctest_build(BUILD "${BINARY_DIR}" RETURN_VALUE _build_ret)
ctest_submit(PARTS Build RETURN_VALUE _submit_ret)

handle_error("Build" _build_ret)

ctest_test(BUILD "${BINARY_DIR}" RETURN_VALUE _test_ret)
ctest_submit(PARTS Test RETURN_VALUE _submit_ret)

handle_error("Testing" _test_ret)

ctest_submit(PARTS Done RETURN_VALUE _submit_ret)
EOF

verbose-run ctest ${CDASH_ARGS} -S dashboard.cmake -VV
