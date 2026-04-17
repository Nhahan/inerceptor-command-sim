if(NOT DEFINED ICSS_SERVER)
    message(FATAL_ERROR "ICSS_SERVER is required")
endif()

if(NOT DEFINED EXPECTED)
    message(FATAL_ERROR "EXPECTED is required")
endif()

set(arg_list)
if(DEFINED ARGS AND NOT ARGS STREQUAL "")
    string(REPLACE "|" ";" arg_list "${ARGS}")
endif()

execute_process(
    COMMAND "${ICSS_SERVER}" ${arg_list}
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error_output
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_STRIP_TRAILING_WHITESPACE)

set(combined_output "${output}")
if(NOT error_output STREQUAL "")
    if(NOT combined_output STREQUAL "")
        string(APPEND combined_output "\n")
    endif()
    string(APPEND combined_output "${error_output}")
endif()

message("${combined_output}")

if(result EQUAL 0)
    message(FATAL_ERROR "expected server command to fail")
endif()

string(FIND "${combined_output}" "${EXPECTED}" expected_index)
if(expected_index EQUAL -1)
    message(FATAL_ERROR "expected message not found: ${EXPECTED}")
endif()
