add_contract(bos.oracle bos.oracle ${CMAKE_CURRENT_SOURCE_DIR}/src/bos.oracle.cpp)

target_include_directories(bos.oracle
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)


set_target_properties(bos.oracle
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

