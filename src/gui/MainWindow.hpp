#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QSettings>

class QTabWidget;
class QToolBar;
class QStatusBar;
class QAction;
class QMenu;
class QSplitter;
class QTreeWidget;
class QListWidget;
class QProgressBar;
class QLabel;
class QPushButton;
class QLineEdit;
class QTextEdit;
class QComboBox;
class QCheckBox;
class QSpinBox;
class QDoubleSpinBox;
class QGroupBox;
class QVBoxLayout;
class QHBoxLayout;
class QFormLayout;
class QScrollArea;
class QFrame;
class QWidget;
class QThread;
class QTimer;
class QFileDialog;
class QMessageBox;
class QMenuBar;

namespace gcanner {

class ScanWidget;
class ResultsWidget;
class SettingsDialog;
class AboutDialog;
class HardwareBrowserWidget;
class ScanWorker;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onNewScan();
    void onOpenResults();
    void onSaveResults();
    void onSettings();
    void onHardwareBrowser();
    void onAbout();
    void onScanProgress(int progress, const QString& status);
    void onScanFinished(const QString& resultPath);
    void onScanError(const QString& error);
    void onThemeChanged(bool dark);
    void updateWindowTitle();

private:
    void setupUI();
    void setupMenus();
    void setupToolBar();
    void setupStatusBar();
    void setupCentralWidget();
    void setupConnections();
    void loadSettings();
    void saveSettings();
    void applyTheme(bool dark);
    void showScanView();
    void showResultsView();
    void showHardwareView();

    QWidget* createScanPage();
    QWidget* createResultsPage();
    QWidget* createHardwarePage();
    QWidget* createWelcomePage();

    ScanWidget* m_scanWidget = nullptr;
    ResultsWidget* m_resultsWidget = nullptr;
    HardwareBrowserWidget* m_hardwareWidget = nullptr;
    QStackedWidget* m_stackedWidget = nullptr;
    QTabWidget* m_tabWidget = nullptr;

    QToolBar* m_toolBar = nullptr;
    QStatusBar* m_statusBar = nullptr;
    QProgressBar* m_progressBar = nullptr;
    QLabel* m_statusLabel = nullptr;

    QAction* m_actNewScan = nullptr;
    QAction* m_actOpenResults = nullptr;
    QAction* m_actSaveResults = nullptr;
    QAction* m_actSettings = nullptr;
    QAction* m_actHardwareBrowser = nullptr;
    QAction* m_actAbout = nullptr;
    QAction* m_actQuit = nullptr;
    QAction* m_actDarkTheme = nullptr;

    SettingsDialog* m_settingsDialog = nullptr;
    AboutDialog* m_aboutDialog = nullptr;
    ScanWorker* m_scanWorker = nullptr;
    QThread* m_scanThread = nullptr;

    QString m_currentResultPath;
    bool m_darkTheme = true;
    QSettings m_settings;
};

} // namespace gcanner