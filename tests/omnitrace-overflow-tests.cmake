# -------------------------------------------------------------------------------------- #
#
# overflow tests
#
# -------------------------------------------------------------------------------------- #

set(_overflow_environment
    "${_base_environment}"
    "OMNITRACE_VERBOSE=2"
    "OMNITRACE_SAMPLING_CPUTIME=OFF"
    "OMNITRACE_SAMPLING_REALTIME=OFF"
    "OMNITRACE_SAMPLING_OVERFLOW=ON"
    "OMNITRACE_SAMPLING_OVERFLOW_EVENT=PERF_COUNT_HW_INSTRUCTIONS"
    "OMNITRACE_SAMPLING_OVERFLOW_FREQ=10000"
    "OMNITRACE_DEBUG_THREADING_GET_ID=ON")

if(omnitrace_perf_event_paranoid LESS_EQUAL 3
   OR omnitrace_cap_sys_admin EQUAL 0
   OR omnitrace_cap_perfmon EQUAL 0)
    omnitrace_add_test(
        SKIP_BASELINE
        NAME overflow
        TARGET parallel-overhead
        RUN_ARGS 20 2 50
        REWRITE_ARGS -e -v 2
        RUNTIME_ARGS -e -v 1
        ENVIRONMENT "${_overflow_environment}"
        LABELS "perf;overflow")
endif()
