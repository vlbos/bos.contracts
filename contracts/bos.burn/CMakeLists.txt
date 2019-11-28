add_contract(bos.burn bos.burn ${CMAKE_CURRENT_SOURCE_DIR}/src/bos.burn.cpp)

target_include_directories(bos.burn
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)


set_target_properties(bos.burn
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
