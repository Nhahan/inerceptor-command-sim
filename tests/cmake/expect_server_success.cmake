if(NOT DEFINED ICSS_SERVER)
    message(FATAL_ERROR "ICSS_SERVER is required")
endif()

set(arg_list)
if(DEFINED ARGS AND NOT ARGS STREQUAL "")
    string(REPLACE "|" ";" arg_list "${ARGS}")
endif()

set(expected_list)
if(DEFINED EXPECTED AND NOT EXPECTED STREQUAL "")
    string(REPLACE "|" ";" expected_list "${EXPECTED}")
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

if(NOT result EQUAL 0)
    message(FATAL_ERROR "expected server command to succeed")
endif()

foreach(expected IN LISTS expected_list)
    string(FIND "${combined_output}" "${expected}" expected_index)
    if(expected_index EQUAL -1)
        message(FATAL_ERROR "expected output fragment not found: ${expected}")
    endif()
endforeach()
