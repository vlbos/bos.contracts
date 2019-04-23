add_contract(bos.consumer bos.consumer ${CMAKE_CURRENT_SOURCE_DIR}/src/bos.consumer.cpp)

target_include_directories(bos.consumer
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(bos.consumer
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
