set(TARGET AutoIntegrator)

add_library(${TARGET} SHARED
    dllmain.cpp
)

target_include_directories(${TARGET} PRIVATE .)
target_link_libraries(${TARGET} PUBLIC UE4SS)
add_custom_command(TARGET ${TARGET} POST_BUILD
     COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${TARGET}> "../../out/${TARGET}/dlls/main.dll"
)
