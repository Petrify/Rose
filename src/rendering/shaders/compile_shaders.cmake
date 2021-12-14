# Source from https://www.reddit.com/r/vulkan/comments/kbaxlz/what_is_your_workflow_when_compiling_shader_files/gfg0s3s/

set(SHADER_SRC_DIR ${CMAKE_CURRENT_LIST_DIR})  

function(add_shaders TARGET)
    find_program(GLSLC glslc)

    foreach(SHADER IN LISTS ARGN)
        set(current-shader-path ${SHADER_SRC_DIR}/${SHADER})
        set(current-output-path ${CMAKE_BINARY_DIR}/shaders/${SHADER}.spv)

        # Add a custom command to compile GLSL to SPIR-V.
        get_filename_component(current-output-dir ${current-output-path} DIRECTORY)
        file(MAKE_DIRECTORY ${current-output-dir})

        add_custom_command(
            OUTPUT ${current-output-path}
            COMMAND ${GLSLC} -o ${current-output-path} ${current-shader-path}
            DEPENDS ${current-shader-path}
            IMPLICIT_DEPENDS CXX ${current-shader-path}
            VERBATIM)

        # Make sure our build depends on this output.
        set_source_files_properties(${current-output-path} PROPERTIES GENERATED TRUE)
        target_sources(${TARGET} PRIVATE ${current-output-path})
    endforeach()
    
endfunction(add_shaders)