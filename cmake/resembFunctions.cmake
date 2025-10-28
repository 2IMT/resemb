# resembFunctions.cmake
#
# Provides resemb_embed(), which generates `resemb` implementation.
#
# Usage:
#   resemb_embed(
#     TARGET <target_name>
#     ASSETS <file1> <file2> ...
#     [STRIP_PREFIX <prefix_path>]
#   )

function(resemb_embed)
    cmake_parse_arguments(RESEMB "" "TARGET;STRIP_PREFIX" "ASSETS" ${ARGN})

    if(NOT RESEMB_TARGET)
        message(FATAL_ERROR "resemb_embed: TARGET is required")
    endif()

    if(NOT RESEMB_ASSETS)
        message(FATAL_ERROR "resemb_embed: ASSETS is required")
    endif()

    if(TARGET resemb_gen)
        set(RESEMB_GEN_EXECUTABLE resemb_gen)
    else()
        find_program(RESEMB_GEN_EXECUTABLE resemb_gen
            HINTS PATH
            REQUIRED
        )
    endif()

    set(output_dir "${CMAKE_CURRENT_BINARY_DIR}/resemb/${RESEMB_TARGET}")
    file(MAKE_DIRECTORY "${output_dir}")
    set(output_c "${output_dir}/resemb_impl.c")

    set(cmd ${RESEMB_GEN_EXECUTABLE} --output ${output_c})
    if (RESEMB_STRIP_PREFIX)
        list(APPEND cmd --strip-prefix "${RESEMB_STRIP_PREFIX}")
    endif()
    list(APPEND cmd ${RESEMB_ASSETS})

    add_custom_command(
        OUTPUT "${output_c}"
        COMMAND ${cmd}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS ${RESEMB_ASSETS} ${RESEMB_GEN_EXECUTABLE}
        COMMENT "Generating `resemb` resources for `${RESEMB_TARGET}`"
        VERBATIM
    )

    target_sources(${RESEMB_TARGET} PRIVATE "${output_c}")
    target_link_libraries(${RESEMB_TARGET} PRIVATE resemb::resemb)
endfunction()
