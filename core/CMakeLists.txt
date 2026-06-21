# ADR-0003: ec-core is a GUI-agnostic static library. It may depend on Qt Core
# (QByteArray, later QtNetwork) but never on Qt Widgets.

add_library(ec-core STATIC
    ec/codes.h
    ec/tag.h
    ec/packet.h
    ec/codec.cpp
)

target_include_directories(ec-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ec-core PUBLIC Qt6::Core)
target_compile_features(ec-core PUBLIC cxx_std_23)
