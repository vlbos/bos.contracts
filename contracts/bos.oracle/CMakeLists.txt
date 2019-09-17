add_contract(bos.oracle bos.oracle ${CMAKE_CURRENT_SOURCE_DIR}/src/bos.oracle.cpp)

target_include_directories(bos.oracle
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)


set_target_properties(bos.oracle
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")


add_contract(consumer.contract consumer.contract ${CMAKE_CURRENT_SOURCE_DIR}/src/example/consumer.contract.cpp)

target_include_directories(consumer.contract
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(consumer.contract
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/consumer.contract")