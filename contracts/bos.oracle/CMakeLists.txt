add_contract(bos.oracle bos.oracle ${CMAKE_CURRENT_SOURCE_DIR}/src/bos.oracle.cpp)

target_include_directories(bos.oracle
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)


set_target_properties(bos.oracle
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

add_contract(bos.oraclize bos.oraclize ${CMAKE_CURRENT_SOURCE_DIR}/src/bos.oraclize.cpp)

target_include_directories(bos.oraclize
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(bos.oraclize
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

add_contract(bos.oraclized bos.oraclized ${CMAKE_CURRENT_SOURCE_DIR}/src/bos.oraclized.cpp)

target_include_directories(bos.oraclized
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(bos.oraclized
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
