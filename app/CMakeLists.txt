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
    searchpanel.h
    searchpanel.cpp
    servertablemodel.h
    servertablemodel.cpp
    sharedfilemodel.h
    sharedfilemodel.cpp
    sourcetablemodel.h
    sourcetablemodel.cpp
    preferencesdialog.h
    preferencesdialog.cpp
    speedgraph.h
    speedgraph.cpp
    addserverdialog.h
    addserverdialog.cpp
    resources.qrc
)

target_link_libraries(amule-remote-qt PRIVATE ec-core Qt6::Widgets Qt6::Svg)
