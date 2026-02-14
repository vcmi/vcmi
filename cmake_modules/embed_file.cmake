# CMake script to embed multiple files as C++ string constants
# Usage: cmake -DINPUT_FILES="file1.txt|file2.yaml" -DOUTPUT_FILE=output.h -P embed_file.cmake

if(NOT DEFINED INPUT_FILES)
    message(FATAL_ERROR "INPUT_FILES must be defined")
endif()

if(NOT DEFINED OUTPUT_FILE)
    message(FATAL_ERROR "OUTPUT_FILE must be defined")
endif()

# Convert pipe-separated string to list
string(REPLACE "|" ";" INPUT_FILES_LIST "${INPUT_FILES}")

# Start building the header content
set(HEADER_CONTENT "// Auto-generated file - DO NOT EDIT\n")
set(HEADER_CONTENT "${HEADER_CONTENT}// Generated from embedded files\n\n")
set(HEADER_CONTENT "${HEADER_CONTENT}#pragma once\n\n")
set(HEADER_CONTENT "${HEADER_CONTENT}namespace EmbeddedFiles\n{\n")

# Process each input file
foreach(INPUT_FILE ${INPUT_FILES_LIST})
    if(NOT EXISTS "${INPUT_FILE}")
        message(FATAL_ERROR "Input file does not exist: ${INPUT_FILE}")
    endif()
    
    # Read the input file
    file(READ "${INPUT_FILE}" FILE_CONTENT)

    # Escape special characters for C++ string literal
    string(REPLACE "\\" "\\\\" FILE_CONTENT "${FILE_CONTENT}")
    string(REPLACE "\"" "\\\"" FILE_CONTENT "${FILE_CONTENT}")
    string(REPLACE "\n" "\\n\"\n\t\"" FILE_CONTENT "${FILE_CONTENT}")

    # Get the base name for the variable
    get_filename_component(VAR_NAME "${INPUT_FILE}" NAME_WE)
    string(TOUPPER "${VAR_NAME}" VAR_NAME)
    string(REPLACE "-" "_" VAR_NAME "${VAR_NAME}")

    # Add to header content
    set(HEADER_CONTENT "${HEADER_CONTENT}\tconst char* ${VAR_NAME}_CONTENT = \"${FILE_CONTENT}\";\n\n")
    
    message(STATUS "Embedded ${INPUT_FILE} as ${VAR_NAME}_CONTENT")
endforeach()

# Close the namespace
set(HEADER_CONTENT "${HEADER_CONTENT}}\n")

# Write the output file
file(WRITE "${OUTPUT_FILE}" "${HEADER_CONTENT}")

message(STATUS "Generated ${OUTPUT_FILE}")
