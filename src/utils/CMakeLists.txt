add_library(rtu_utils STATIC
    Logger.h
    Logger.cpp
    AppConfig.h
)

set_target_properties(rtu_utils PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(rtu_utils PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(rtu_utils PUBLIC Qt6::Core)
