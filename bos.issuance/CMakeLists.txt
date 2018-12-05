add_contract(bos.issuance bos.issuance ${CMAKE_CURRENT_SOURCE_DIR}/src/bos.issuance.cpp)
target_include_directories(bos.issuance.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(bos.issuance.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
