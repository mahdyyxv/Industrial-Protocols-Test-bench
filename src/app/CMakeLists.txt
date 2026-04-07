qt_add_executable(Project1RTU
    main.cpp
    Application.h
    Application.cpp
)

target_include_directories(Project1RTU PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(Project1RTU PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    rtu_ui
    rtu_uiplugin      # static QML plugin — must be explicit for Q_IMPORT_QML_PLUGIN
    rtu_services
    rtu_utils
)

# ── Windows: copy Qt DLLs next to the executable after each build ─────────
# Without this the app will fail to launch with "DLL not found" errors.
if(WIN32)
    find_program(_windeployqt windeployqt HINTS "${Qt6_DIR}/../../../bin")
    if(_windeployqt)
        add_custom_command(TARGET Project1RTU POST_BUILD
            COMMAND "${_windeployqt}"
                    --qmldir "${CMAKE_SOURCE_DIR}/src/ui/qml"
                    --dir    "$<TARGET_FILE_DIR:Project1RTU>"
                    "$<TARGET_FILE:Project1RTU>"
            COMMENT "Running windeployqt..."
            VERBATIM
        )
    else()
        message(WARNING "windeployqt not found — Qt DLLs will not be auto-deployed.\n"
            "   Run manually: windeployqt --qmldir src/ui/qml <path/to/Project1RTU.exe>")
    endif()
endif()
