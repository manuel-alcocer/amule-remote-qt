# ADR-0002/0003: the Qt Widgets GUI application. Links the GUI-agnostic
# ec-core library and Qt Widgets.

add_executable(amule-remote-qt
    main.cpp
    mainwindow.h
    mainwindow.cpp
    downloadtablemodel.h
    downloadtablemodel.cpp
    progressbardelegate.h
    progressbardelegate.cpp
    searchresultmodel.h
    searchresultmodel.cpp
    searchfilterproxy.h
    searchfilterproxy.cpp
    savedsearches.h
    savedsearches.cpp
    searchpanel.h
    searchpanel.cpp
    servertablemodel.h
    servertablemodel.cpp
    sharedfilemodel.h
    sharedfilemodel.cpp
    sourcetablemodel.h
    sourcetablemodel.cpp
    preferencespanel.h
    preferencespanel.cpp
    speedgraph.h
    speedgraph.cpp
    addserverdialog.h
    addserverdialog.cpp
    i18n.h
    i18n.cpp
    resources.qrc
    i18n.qrc
)

target_link_libraries(amule-remote-qt PRIVATE ec-core Qt6::Widgets Qt6::Svg)
target_compile_definitions(amule-remote-qt PRIVATE APP_VERSION="${PROJECT_VERSION}")

# Install rules for packaging (AppImage builds an AppDir from these).
include(GNUInstallDirs)
install(TARGETS amule-remote-qt RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${CMAKE_SOURCE_DIR}/packaging/amule-remote-qt.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/icons/app.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
        RENAME amule-remote-qt.svg)
