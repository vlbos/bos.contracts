add_contract(bos.pegtoken bos.pegtoken ${CMAKE_CURRENT_SOURCE_DIR}/src/bos.pegtoken.cpp)
target_include_directories(bos.pegtoken.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(bos.pegtoken.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
