QT += gui quick quickcontrols2

CONFIG += link_pkgconfig
PKGCONFIG += alsa

QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -O3

SOURCES += src/main.cpp \
    src/alsaproxy.cpp

DISTFILES += qml/main.qml \
    qml/Indicator.qml

RESOURCES += \
    resources.qrc

HEADERS += \
    src/alsaproxy.h

lupdate_only {
    SOURCES = qml/*.qml
}

TRANSLATIONS += i18n/AlsaMidiRemapper_eo.ts \
    i18n/AlsaMidiRemapper_es.ts \
    i18n/AlsaMidiRemapper_fr.ts \
    i18n/AlsaMidiRemapper_ru.ts
