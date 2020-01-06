add_contract(bos.bridge bos.bridge ${CMAKE_CURRENT_SOURCE_DIR}/src/bos.bridge.cpp)

target_include_directories(bos.bridge
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)


set_target_properties(bos.bridge
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

