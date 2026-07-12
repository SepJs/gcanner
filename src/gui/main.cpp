#include <QApplication>
#include <QStyleFactory>
#include <QIcon>
#include <QDir>
#include <QCommandLineParser>
#include <QSplashScreen>
#include <QPixmap>
#include <QTimer>
#include <QMessageBox>
#include <QTranslator>
#include <QLocale>
#include <QScreen>

#include "MainWindow.hpp"

int main(int argc, char* argv[]) {
    // High DPI support
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);
    app.setApplicationName("gcanner");
    app.setApplicationDisplayName("gcanner - Game System Requirements Analyzer");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("gcanner");
    app.setOrganizationDomain("gcanner.org");

    // Set Fusion style for consistent cross-platform look
    app.setStyle(QStyleFactory::create("Fusion"));

    // Command line parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Premium Game System Requirements Analyzer");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption scanOption(QStringList() << "s" << "scan", "Scan directory", "path");
    parser.addOption(scanOption);

    QCommandLineOption outputOption(QStringList() << "o" << "output", "Output file", "file");
    parser.addOption(outputOption);

    QCommandLineOption formatOption(QStringList() << "f" << "format", "Output format (json, csv, html, markdown)", "format");
    parser.addOption(formatOption);

    QCommandLineOption noGuiOption("no-gui", "Run in CLI mode (if available)");
    parser.addOption(noGuiOption);

    parser.process(app);

    // Check for CLI mode
    if (parser.isSet(noGuiOption) || parser.isSet(scanOption)) {
        // TODO: Implement CLI mode
        QMessageBox::information(nullptr, "gcanner", "CLI mode not yet implemented. Use the GUI instead.");
        return 0;
    }

    // Create and show splash screen
    QPixmap splashPixmap(400, 300);
    splashPixmap.fill(QColor("#1e1e2e"));
    QSplashScreen splash(splashPixmap);
    splash.showMessage("Loading gcanner...", Qt::AlignBottom | Qt::AlignCenter, Qt::white);
    splash.show();
    app.processEvents();

    // Load translator if needed
    QTranslator translator;
    QString locale = QLocale::system().name();
    if (translator.load("gcanner_" + locale, ":/translations")) {
        app.installTranslator(&translator);
    }

    // Create main window
    gcanner::MainWindow mainWindow;

    // Simulate loading
    QTimer::singleShot(800, [&splash, &mainWindow]() {
        splash.finish(&mainWindow);
        mainWindow.show();
    });

    // Apply any command line arguments
    if (parser.isSet(scanOption)) {
        QString path = parser.value(scanOption);
        // TODO: Auto-start scan with this path
    }

    return app.exec();
}