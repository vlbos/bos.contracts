add_contract(bos.dappuser bos.dappuser ${CMAKE_CURRENT_SOURCE_DIR}/src/bos.dappuser.cpp)

target_include_directories(bos.dappuser
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(bos.dappuser
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
