add_contract(redpacket redpacket ${CMAKE_CURRENT_SOURCE_DIR}/src/redpacket.cpp)
target_include_directories(redpacket
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(redpacket
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

