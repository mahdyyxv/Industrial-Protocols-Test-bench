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
