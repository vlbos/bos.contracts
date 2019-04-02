add_contract(personal.bos personal.bos ${CMAKE_CURRENT_SOURCE_DIR}/src/personal.bos.cpp)
target_include_directories(personal.bos
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(personal.bos
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
