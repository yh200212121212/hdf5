# grepTest.cmake executes a command and captures the output in a file. File is then compared
# against a reference file. Exit status of command can also be compared.

# arguments checking
IF (NOT TEST_PROGRAM)
  MESSAGE (FATAL_ERROR "Require TEST_PROGRAM to be defined")
ENDIF (NOT TEST_PROGRAM)
#IF (NOT TEST_ARGS)
#  MESSAGE (STATUS "Require TEST_ARGS to be defined")
#ENDIF (NOT TEST_ARGS)
IF (NOT TEST_FOLDER)
  MESSAGE ( FATAL_ERROR "Require TEST_FOLDER to be defined")
ENDIF (NOT TEST_FOLDER)
IF (NOT TEST_OUTPUT)
  MESSAGE (FATAL_ERROR "Require TEST_OUTPUT to be defined")
ENDIF (NOT TEST_OUTPUT)
#IF (NOT TEST_EXPECT)
#  MESSAGE (STATUS "Require TEST_EXPECT to be defined")
#ENDIF (NOT TEST_EXPECT)
IF (NOT TEST_FILTER)
  MESSAGE (STATUS "Require TEST_FILTER to be defined")
ENDIF (NOT TEST_FILTER)
IF (NOT TEST_REFERENCE)
  MESSAGE (FATAL_ERROR "Require TEST_REFERENCE to be defined")
ENDIF (NOT TEST_REFERENCE)

MESSAGE (STATUS "COMMAND: ${TEST_PROGRAM} ${TEST_ARGS}")

# run the test program, capture the stdout/stderr and the result var
EXECUTE_PROCESS (
    COMMAND ${TEST_PROGRAM} ${TEST_ARGS}
    WORKING_DIRECTORY ${TEST_FOLDER}
    RESULT_VARIABLE TEST_RESULT
    OUTPUT_FILE ${TEST_OUTPUT}
    ERROR_FILE ${TEST_OUTPUT}.err
    OUTPUT_VARIABLE TEST_ERROR
    ERROR_VARIABLE TEST_ERROR
)

MESSAGE (STATUS "COMMAND Result: ${TEST_RESULT}")
MESSAGE (STATUS "COMMAND Error: ${TEST_ERROR}")

# now grep the output with the reference
FILE (READ ${TEST_FOLDER}/${TEST_OUTPUT} TEST_STREAM)

# TEST_REFERENCE should always be matched
STRING(REGEX MATCH "${TEST_REFERENCE}" TEST_MATCH ${TEST_STREAM}) 
STRING(COMPARE EQUAL "${TEST_REFERENCE}" "${TEST_MATCH}" TEST_RESULT) 
IF (${TEST_RESULT} STREQUAL "0")
  MESSAGE (FATAL_ERROR "Failed: The output of ${TEST_PROGRAM} did not contain ${TEST_REFERENCE}")
ENDIF (${TEST_RESULT} STREQUAL "0")

STRING(REGEX MATCH "${TEST_FILTER}" TEST_MATCH ${TEST_STREAM}) 
IF (${TEST_EXPECT} STREQUAL "1")
  # TEST_EXPECT (1) interperts TEST_FILTER as NOT to match
  STRING(LENGTH "${TEST_MATCH}" TEST_RESULT) 
  IF (NOT ${TEST_RESULT} STREQUAL "0")
    MESSAGE (FATAL_ERROR "Failed: The output of ${TEST_PROGRAM} did contain ${TEST_FILTER}")
  ENDIF (NOT ${TEST_RESULT} STREQUAL "0")
ENDIF (${TEST_EXPECT} STREQUAL "0")

# everything went fine...
MESSAGE ("Passed: The output of ${TEST_PROGRAM} matched")

