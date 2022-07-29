#!/bin/bash -e

SCRIPT_DIR=$(realpath $(dirname ${BASH_SOURCE[0]}))
cd $(dirname ${SCRIPT_DIR})
echo -e "Working directory: $(pwd)"

error-message()
{
    echo -e "\nError! ${@}\n"
    exit -1
}

verbose-run()
{
    echo -e "\n##### Executing \"${@}\"... #####\n"
    eval $@
}

if [ -d "$(realpath /tmp)" ]; then
    : ${TMPDIR:=/tmp}
    export TMPDIR
fi

: ${SOURCE_DIR:=$(dirname ${SCRIPT_DIR})}
: ${INSTALL_DIR:=/opt/omnitrace}
: ${WORKING_DIR:=$(mktemp -t -d omnitrace-test-install-XXXX)}

usage()
{
    print_option() { printf "    --%-10s %-24s     %s (default: %s)\n" "${1}" "${2}" "${3}" "${4}"; }
    echo "Options:"
    print_option source-dir "<PATH>" "Path to source directory" "${SOURCE_DIR}"
    print_option working-dir "<PATH>" "Path to working directory" "${WORKING_DIR}"
    print_option install-dir "<PATH>" "Location of omnitrace installation" "${INSTALL_DIR}"
}

while [[ $# -gt 0 ]]
do
    ARG=${1}
    shift

    VAL=""
    while [[ $# -gt 0 ]]
    do
        VAL=${1}
        shift
        break
    done

    if [ -z "${VAL}" ]; then
        echo "Error! Missing value for argument \"${ARG}\""
        usage
        exit -1
    fi

    case "${ARG}" in
        --source-dir)
            SOURCE_DIR=${VAL}
            ;;
        --working-dir)
            WORKING_DIR=${VAL}
            ;;
        --install-dir)
            INSTALL_DIR=${VAL}
            ;;
        *)
            echo -e "Error! Unknown option : ${ARG}"
            usage
            exit -1
            ;;
    esac
done

if [ ! -d ${WORKING_DIR} ]; then
    verbose-run mkdir -p ${WORKING_DIR}
fi

verbose-run pushd ${WORKING_DIR}

verbose-run pwd

cat << EOF > omnitrace-test.cfg
OMNITRACE_DEBUG                 = OFF
OMNITRACE_VERBOSE               = 1
OMNITRACE_DL_VERBOSE            = 1

OMNITRACE_USE_TIMEMORY          = ON
OMNITRACE_USE_PERFETTO          = ON
OMNITRACE_USE_SAMPLING          = ON
OMNITRACE_USE_PROCESS_SAMPLING  = ON

OMNITRACE_OUTPUT_PATH           = omnitrace-tests-output
OMNITRACE_OUTPUT_PREFIX         = %tag%/
OMNITRACE_COUT_OUTPUT           = ON
OMNITRACE_TIME_OUTPUT           = OFF
OMNITRACE_USE_PID               = OFF

OMNITRACE_TIMEMORY_COMPONENTS   = cpu_clock cpu_util current_peak_rss kernel_mode_time monotonic_clock monotonic_raw_clock network_stats num_io_in num_io_out num_major_page_faults num_minor_page_faults page_rss peak_rss priority_context_switch process_cpu_clock process_cpu_util read_bytes read_char system_clock thread_cpu_clock thread_cpu_util timestamp trip_count user_clock user_mode_time virtual_memory voluntary_context_switch wall_clock written_bytes written_char

OMNITRACE_SAMPLING_FREQ         = 20
OMNITRACE_MAX_WIDTH             = 180
EOF

set +e

if [[ -n "$(which omnitrace-perfetto-traced)" && -n "$(which omnitrace-perfetto)" ]]; then
    OMNI_TRACED=$(which omnitrace-perfetto-traced)
    OMNI_PERFETTO=$(which omnitrace-perfetto)
    echo "OMNITRACE_PERFETTO_BACKEND=system" >> omnitrace-test.cfg
elif [[ -n "$(which traced)" && -n "$(which perfetto)" ]]; then
    OMNI_TRACED=$(which traced)
    OMNI_PERFETTO=$(which perfetto)
    echo "OMNITRACE_PERFETTO_BACKEND=system" >> omnitrace-test.cfg
fi

if [ -n "$(which omnitrace-python)" ]; then
    OMNI_PYTHON=$(which omnitrace-python)
fi

set -e

start-perfetto-system()
{
    run-verbose ${OMNI_TRACED} --background
    run-verbose ${OMNI_PERFETTO} --out test-perfetto-trace.proto --txt -c ${SOURCE_DIR}/omnitrace.cfg
}

run-instr()
{
    local CMD=${1}
    shift
    local ARG=${@}

    verbose-run omnitrace -e -v 1 -o ${CMD}.inst --simulate -- ${CMD} ${ARG}
    verbose-run omnitrace -e -v 1 --simulate -- ${CMD} ${ARG}
    verbose-run omnitrace -e -v 1 -o ${CMD}.inst -- ${CMD} ${ARG}

    for i in omnitrace-tests-output/${CMD}.inst/*
    do
        verbose-run cat ${i}
    done

    verbose-run ./${CMD}.inst ${ARG}
    verbose-run omnitrace -e -v 1 -- ${CMD} ${ARG}
}

export OMNITRACE_CONFIG_FILE=${PWD}/omnitrace-test.cfg
source ${INSTALL_DIR}/share/omnitrace/setup-env.sh

verbose-run ls ${PWD}
verbose-run cat ${OMNITRACE_CONFIG_FILE}
verbose-run env

verbose-run which omnitrace
verbose-run ldd $(which omnitrace)

verbose-run which omnitrace-avail
verbose-run ldd $(which omnitrace-avail)

verbose-run which omnitrace-critical-trace
verbose-run ldd $(which omnitrace-critical-trace)

verbose-run omnitrace-avail --help
verbose-run omnitrace-avail -a

if [ -n "${OMNI_PYTHON}" ]; then
    verbose-run echo ${OMNI_PYTHON}
    verbose-run which omnitrace-python
    verbose-run omnitrace-python --help
    verbose-run omnitrace-python -b -- ${SOURCE_DIR}/examples/python/builtin.py -v 10 -n 3
fi

verbose-run omnitrace --help

if [ -n "${OMNI_TRACED}" ]; then
    verbose-run echo ${OMNI_TRACED}
    verbose-run echo ${OMNI_PERFETTO}
    verbose-run start-perfetto-system
fi

run-instr ls
export OMNITRACE_USE_SAMPLING=OFF
run-instr sleep 3

if [ -n "${OMNI_TRACED}" ]; then
    sleep 1
    verbose-run du -k ${PWD}/test-perfetto-trace.proto
    verbose-run python3 ${SOURCE_DIR}/tests/validate-perfetto-proto.py -i ${PWD}/test-perfetto-trace.proto -p
fi

verbose-run find ${WORKING_DIR} -type f

verbose-run echo "Test success : ${WORKING_DIR}"
