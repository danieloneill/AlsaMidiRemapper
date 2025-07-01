#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTranslator>
#include <QQuickStyle>
#include <QLoggingCategory>

#include "alsaproxy.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setOrganizationName("oneill");
    app.setApplicationName("AlsaMidiRemapper");
    app.setOrganizationDomain("oneill.app");
    app.setQuitOnLastWindowClosed(false);

    QLoggingCategory::defaultCategory()->setEnabled(QtDebugMsg, true);
    QQuickStyle::setStyle("Material");

    QTranslator translator;
    if (translator.load(QLocale(), "AlsaMidiRemapper", "_", ":/i18n"))
        app.installTranslator(&translator);

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    app.setQuitOnLastWindowClosed(true);

    AlsaProxy p;
    engine.rootContext()->setContextProperty("AlsaProxy", &p);

    engine.load(":/qml/main.qml");

    return app.exec();
}
