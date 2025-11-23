set(TARGET AutoIntegrator)

add_library(${TARGET} SHARED
    dllmain.cpp
)

target_include_directories(${TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/OpenSSL/3.4.1/include)

if(WIN32)
  target_link_libraries(${TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/OpenSSL/3.4.1/lib/libssl_static.lib)
  target_link_libraries(${TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/OpenSSL/3.4.1/lib/libcrypto_static.lib)
endif()

target_include_directories(${TARGET} PRIVATE .)
target_link_libraries(${TARGET} PUBLIC UE4SS)
add_custom_command(TARGET ${TARGET} POST_BUILD
     COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${TARGET}> "../../out/${TARGET}/dlls/main.dll"
)
