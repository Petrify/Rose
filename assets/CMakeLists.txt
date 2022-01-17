# simply copy over all files and subdirectories in this folder  

file(GLOB assets RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} * )

# use this to exclude certain files
list(FILTER assets EXCLUDE REGEX CMakeLists.txt$)

foreach(asset IN LISTS assets)
    file(COPY ${asset} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endforeach()