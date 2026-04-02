add_library(rtu_core STATIC
    models/ProtocolFrame.h
    protocols/IProtocol.h
    protocols/ProtocolRegistry.h
    engine/ITestSession.h
    engine/TestEngine.h
)

# Header-only for now — implementations added in Core Engineer phase
set_target_properties(rtu_core PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(rtu_core PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(rtu_core PUBLIC
    Qt6::Core
    Qt6::Network
)

if(Qt6SerialPort_FOUND)
    target_link_libraries(rtu_core PUBLIC Qt6::SerialPort)
endif()
