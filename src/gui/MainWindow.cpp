#include "MainWindow.hpp"
#include "ScanWidget.hpp"
#include "ResultsWidget.hpp"
#include "HardwareBrowserWidget.hpp"
#include "SettingsDialog.hpp"
#include "AboutDialog.hpp"
#include "ScanWorker.hpp"

#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QProgressBar>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QTabWidget>
#include <QSplitter>
#include <QSettings>
#include <QTimer>
#include <QThread>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStyleFactory>
#include <QScreen>

namespace gcanner {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_settings("gcanner", "gcanner")
{
    setWindowTitle("gcanner - Game System Requirements Analyzer");
    resize(1400, 900);
    setMinimumSize(1000, 700);

    loadSettings();
    applyTheme(m_darkTheme);
    setupUI();
    setupMenus();
    setupToolBar();
    setupStatusBar();
    setupCentralWidget();
    setupConnections();

    // Center on screen
    QScreen* screen = QApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->availableGeometry();
        move((screenGeometry.width() - width()) / 2,
             (screenGeometry.height() - height()) / 2);
    }

    // Show welcome page initially
    showWelcomePage();
}

MainWindow::~MainWindow() {
    saveSettings();
    if (m_scanThread && m_scanThread->isRunning()) {
        m_scanThread->quit();
        m_scanThread->wait();
    }
}

void MainWindow::setupUI() {
    setAnimated(true);
    setDockNestingEnabled(true);
}

void MainWindow::setupMenus() {
    QMenuBar* menuBar = this->menuBar();

    // File Menu
    QMenu* fileMenu = menuBar->addMenu("&File");
    m_actNewScan = fileMenu->addAction(QIcon::fromTheme("document-new"), "&New Scan...", this, &MainWindow::onNewScan);
    m_actNewScan->setShortcut(QKeySequence::New);
    m_actNewScan->setToolTip("Start a new game directory scan");

    fileMenu->addSeparator();

    m_actOpenResults = fileMenu->addAction(QIcon::fromTheme("document-open"), "&Open Results...", this, &MainWindow::onOpenResults);
    m_actOpenResults->setShortcut(QKeySequence::Open);

    m_actSaveResults = fileMenu->addAction(QIcon::fromTheme("document-save"), "&Save Results...", this, &MainWindow::onSaveResults);
    m_actSaveResults->setShortcut(QKeySequence::Save);
    m_actSaveResults->setEnabled(false);

    fileMenu->addSeparator();

    QAction* actExport = fileMenu->addAction(QIcon::fromTheme("document-export"), "&Export Report...", this, [](){
        // TODO: Export to various formats
    });
    actExport->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E));

    fileMenu->addSeparator();

    m_actQuit = fileMenu->addAction(QIcon::fromTheme("application-exit"), "&Quit", qApp, &QApplication::quit);
    m_actQuit->setShortcut(QKeySequence::Quit);

    // View Menu
    QMenu* viewMenu = menuBar->addMenu("&View");
    m_actDarkTheme = viewMenu->addAction("&Dark Theme", this, &MainWindow::onThemeChanged);
    m_actDarkTheme->setCheckable(true);
    m_actDarkTheme->setChecked(m_darkTheme);

    viewMenu->addSeparator();

    QAction* actScanView = viewMenu->addAction("&Scan View", this, &MainWindow::showScanView);
    actScanView->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_1));

    QAction* actResultsView = viewMenu->addAction("&Results View", this, &MainWindow::showResultsView);
    actResultsView->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_2));

    QAction* actHardwareView = viewMenu->addAction("&Hardware Browser", this, &MainWindow::showHardwareView);
    actHardwareView->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_3));

    // Tools Menu
    QMenu* toolsMenu = menuBar->addMenu("&Tools");
    m_actHardwareBrowser = toolsMenu->addAction(QIcon::fromTheme("preferences-system"), "&Hardware Browser", this, &MainWindow::onHardwareBrowser);
    m_actSettings = toolsMenu->addAction(QIcon::fromTheme("preferences-system"), "S&ettings...", this, &MainWindow::onSettings);
    m_actSettings->setShortcut(QKeySequence::Preferences);

    // Help Menu
    QMenu* helpMenu = menuBar->addMenu("&Help");
    m_actAbout = helpMenu->addAction(QIcon::fromTheme("help-about"), "&About gcanner", this, &MainWindow::onAbout);
    helpMenu->addSeparator();

    QAction* actDocumentation = helpMenu->addAction("&Documentation", [](){
        QDesktopServices::openUrl(QUrl("https://github.com/SepJs/gcanner/wiki"));
    });
    QAction* actReportBug = helpMenu->addAction("&Report Issue", [](){
        QDesktopServices::openUrl(QUrl("https://github.com/SepJs/gcanner/issues"));
    });
}

void MainWindow::setupToolBar() {
    m_toolBar = addToolBar("Main Toolbar");
    m_toolBar->setObjectName("MainToolBar");
    m_toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_toolBar->setMovable(false);
    m_toolBar->setIconSize(QSize(24, 24));

    m_toolBar->addAction(m_actNewScan);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_actOpenResults);
    m_toolBar->addAction(m_actSaveResults);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_actHardwareBrowser);
    m_toolBar->addAction(m_actSettings);

    m_toolBar->addWidget(new QWidget()); // Spacer

    // Theme toggle button
    QPushButton* btnTheme = new QPushButton(m_darkTheme ? "☀ Light" : "🌙 Dark");
    btnTheme->setCheckable(true);
    btnTheme->setChecked(m_darkTheme);
    btnTheme->setToolTip("Toggle dark/light theme");
    connect(btnTheme, &QPushButton::toggled, this, &MainWindow::onThemeChanged);
    m_toolBar->addWidget(btnTheme);
}

void MainWindow::setupStatusBar() {
    m_statusBar = statusBar();
    m_statusBar->setObjectName("MainStatusBar");

    m_statusLabel = new QLabel("Ready");
    m_statusLabel->setMinimumWidth(200);
    m_statusBar->addWidget(m_statusLabel, 1);

    m_progressBar = new QProgressBar();
    m_progressBar->setFixedWidth(200);
    m_progressBar->setVisible(false);
    m_progressBar->setTextVisible(true);
    m_statusBar->addPermanentWidget(m_progressBar);

    QLabel* versionLabel = new QLabel("v1.0.0");
    versionLabel->setStyleSheet("color: #888; padding-right: 10px;");
    m_statusBar->addPermanentWidget(versionLabel);
}

void MainWindow::setupCentralWidget() {
    m_stackedWidget = new QStackedWidget(this);
    setCentralWidget(m_stackedWidget);

    // Create pages
    QWidget* welcomePage = createWelcomePage();
    QWidget* scanPage = createScanPage();
    QWidget* resultsPage = createResultsPage();
    QWidget* hardwarePage = createHardwarePage();

    m_stackedWidget->addWidget(welcomePage);    // Index 0
    m_stackedWidget->addWidget(scanPage);       // Index 1
    m_stackedWidget->addWidget(resultsPage);    // Index 2
    m_stackedWidget->addWidget(hardwarePage);   // Index 3
}

void MainWindow::setupConnections() {
    connect(m_actDarkTheme, &QAction::toggled, this, &MainWindow::onThemeChanged);
}

QWidget* MainWindow::createWelcomePage() {
    QWidget* page = new QWidget();
    page->setObjectName("WelcomePage");
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(20);
    layout->setContentsMargins(40, 40, 40, 40);

    // Logo/Title
    QLabel* logoLabel = new QLabel();
    logoLabel->setPixmap(QIcon::fromTheme("gcanner").pixmap(128, 128));
    logoLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(logoLabel);

    QLabel* titleLabel = new QLabel("gcanner");
    titleLabel->setObjectName("TitleLabel");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 48px; font-weight: bold; color: #2c3e50;");
    layout->addWidget(titleLabel);

    QLabel* subtitleLabel = new QLabel("Game System Requirements Analyzer");
    subtitleLabel->setObjectName("SubtitleLabel");
    subtitleLabel->setAlignment(Qt::AlignCenter);
    subtitleLabel->setStyleSheet("font-size: 18px; color: #7f8c8d;");
    layout->addWidget(subtitleLabel);

    layout->addSpacing(20);

    // Feature cards
    QWidget* cardsWidget = new QWidget();
    QHBoxLayout* cardsLayout = new QHBoxLayout(cardsWidget);
    cardsLayout->setSpacing(20);

    auto createCard = [](const QString& icon, const QString& title, const QString& desc) {
        QFrame* card = new QFrame();
        card->setObjectName("FeatureCard");
        card->setFrameShape(QFrame::StyledPanel);
        card->setFixedWidth->setMinimumWidth(250);
        QVBoxLayout* cl = new QVBoxLayout(card);
        cl->setAlignment(Qt::AlignCenter);
        cl->setSpacing(10);

        QLabel* iconLabel = new QLabel(icon);
        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setStyleSheet("font-size: 48px;");
        cl->addWidget(iconLabel);

        QLabel* tLabel = new QLabel(title);
        tLabel->setAlignment(Qt::AlignCenter);
        tLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #2c3e50;");
        cl->addWidget(tLabel);

        QLabel* dLabel = new QLabel(desc);
        dLabel->setAlignment(Qt::AlignCenter);
        dLabel->setWordWrap(true);
        dLabel->setStyleSheet("font-size: 13px; color: #7f8c8d;");
        cl->addWidget(dLabel);

        return card;
    };

    cardsLayout->addWidget(createCard("🔍", "Scan & Analyze", "Deep scan game directories to detect all asset types including textures, models, audio, scripts, shaders, and more."));
    cardsLayout->addWidget(createCard("📊", "Detailed Reports", "Generate comprehensive system requirements with minimum, recommended, high, ultra, and ray-tracing tiers."));
    cardsLayout->addWidget(createCard("🖥️", "Hardware Database", "Browse up-to-date CPU, GPU, RAM, and storage benchmarks with performance-per-dollar recommendations."));
    cardsLayout->addWidget(createCard("📤", "Export & Share", "Export reports as JSON, CSV, HTML, Markdown, or PDF for documentation and sharing."));

    layout->addWidget(cardsWidget);
    layout->addStretch();

    // Action buttons
    QWidget* buttonWidget = new QWidget();
    QHBoxLayout* btnLayout = new QHBoxLayout(buttonWidget);
    btnLayout->setSpacing(15);
    btnLayout->setAlignment(Qt::AlignCenter);

    auto createBtn = [this](const QString& text, const QString& icon, const char* slot, bool primary = false) {
        QPushButton* btn = new QPushButton(text);
        btn->setMinimumSize(180, 50);
        btn->setCursor(Qt::PointingHandCursor);
        if (primary) btn->setObjectName("PrimaryButton");
        connect(btn, &QPushButton::clicked, this, slot);
        return btn;
    };

    btnLayout->addWidget(createBtn("Start New Scan", "🔍", SLOT(onNewScan()), true));
    btnLayout->addWidget(createBtn("Open Results", "📂", SLOT(onOpenResults())));
    btnLayout->addWidget(createBtn("Hardware Browser", "🖥️", SLOT(onHardwareBrowser())));
    btnLayout->addWidget(createBtn("Settings", "⚙️", SLOT(onSettings())));

    layout->addWidget(buttonWidget);

    return page;
}

QWidget* MainWindow::createScanPage() {
    m_scanWidget = new ScanWidget(this);
    connect(m_scanWidget, &ScanWidget::scanStarted, this, [this](){
        m_progressBar->setVisible(true);
        m_progressBar->setValue(0);
        m_statusLabel->setText("Scanning...");
    });
    connect(m_scanWidget, &ScanWidget::scanProgress, this, &MainWindow::onScanProgress);
    connect(m_scanWidget, &ScanWidget::scanFinished, this, &MainWindow::onScanFinished);
    connect(m_scanWidget, &ScanWidget::scanError, this, &MainWindow::onScanError);
    return m_scanWidget;
}

QWidget* MainWindow::createResultsPage() {
    m_resultsWidget = new ResultsWidget(this);
    connect(m_resultsWidget, &ResultsWidget::exportRequested, this, [this](const QString& format){
        // TODO: Implement export
    });
    return m_resultsWidget;
}

QWidget* MainWindow::createHardwarePage() {
    m_hardwareWidget = new HardwareBrowserWidget(this);
    return m_hardwareWidget;
}

void MainWindow::showWelcomePage() {
    m_stackedWidget->setCurrentIndex(0);
}

void MainWindow::showScanView() {
    m_stackedWidget->setCurrentIndex(1);
}

void MainWindow::showResultsView() {
    m_stackedWidget->setCurrentIndex(2);
}

void MainWindow::showHardwareView() {
    m_stackedWidget->setCurrentIndex(3);
}

void MainWindow::onNewScan() {
    showScanView();
    m_scanWidget->startNewScan();
}

void MainWindow::onOpenResults() {
    QString file = QFileDialog::getOpenFileName(this, "Open Results", QDir::homePath(),
        "JSON Files (*.json);;All Files (*)");
    if (!file.isEmpty()) {
        m_resultsWidget->loadResults(file);
        showResultsView();
        m_currentResultPath = file;
        m_actSaveResults->setEnabled(true);
        updateWindowTitle();
    }
}

void MainWindow::onSaveResults() {
    if (m_currentResultPath.isEmpty()) {
        QString file = QFileDialog::getSaveFileName(this, "Save Results", QDir::homePath() + "/gcanner_report.json",
            "JSON Files (*.json);;All Files (*)");
        if (file.isEmpty()) return;
        m_currentResultPath = file;
    }
    m_resultsWidget->saveResults(m_currentResultPath);
    m_statusLabel->setText(QString("Saved to %1").arg(m_currentResultPath));
}

void MainWindow::onSettings() {
    if (!m_settingsDialog) {
        m_settingsDialog = new SettingsDialog(this);
    }
    if (m_settingsDialog->exec() == QDialog::Accepted) {
        loadSettings();
        applyTheme(m_darkTheme);
    }
}

void MainWindow::onHardwareBrowser() {
    showHardwareView();
}

void MainWindow::onAbout() {
    if (!m_aboutDialog) {
        m_aboutDialog = new AboutDialog(this);
    }
    m_aboutDialog->exec();
}

void MainWindow::onScanProgress(int progress, const QString& status) {
    m_progressBar->setValue(progress);
    m_statusLabel->setText(status);
}

void MainWindow::onScanFinished(const QString& resultPath) {
    m_progressBar->setVisible(false);
    m_statusLabel->setText("Scan complete");
    m_currentResultPath = resultPath;
    m_actSaveResults->setEnabled(true);
    m_resultsWidget->loadResults(resultPath);
    showResultsView();
    updateWindowTitle();
}

void MainWindow::onScanError(const QString& error) {
    m_progressBar->setVisible(false);
    m_statusLabel->setText("Scan failed");
    QMessageBox::critical(this, "Scan Error", error);
}

void MainWindow::onThemeChanged(bool dark) {
    m_darkTheme = dark;
    m_actDarkTheme->setChecked(dark);
    applyTheme(dark);
}

void MainWindow::applyTheme(bool dark) {
    QString styleSheet;
    if (dark) {
        styleSheet = R"(
            QMainWindow, QWidget { background-color: #1e1e2e; color: #cdd6f4; }
            QMenuBar { background-color: #181825; color: #cdd6f4; border: none; padding: 4px; }
            QMenuBar::item { background: transparent; padding: 6px 12px; border-radius: 4px; }
            QMenuBar::item:selected { background-color: #313244; }
            QMenu { background-color: #1e1e2e; border: 1px solid #313244; border-radius: 6px; padding: 4px; }
            QMenu::item { padding: 8px 24px; border-radius: 4px; }
            QMenu::item:selected { background-color: #313244; }
            QMenu::separator { height: 1px; background: #313244; margin: 4px 8px; }

            QToolBar { background-color: #181825; border: none; spacing: 8px; padding: 8px; }
            QToolBar::separator { background: #313244; width: 1px; margin: 4px; }
            QToolButton { background: transparent; border: none; border-radius: 6px; padding: 8px 12px; color: #cdd6f4; }
            QToolButton:hover { background-color: #313244; }
            QToolButton:pressed { background-color: #45475a; }

            QStatusBar { background-color: #181825; border-top: 1px solid #313244; color: #a6adc8; }
            QProgressBar { border: none; border-radius: 4px; background-color: #313244; text-align: center; color: #cdd6f4; height: 16px; }
            QProgressBar::chunk { background-color: #89b4fa; border-radius: 4px; }

            QTabWidget::pane { border: 1px solid #313244; border-radius: 6px; background: #1e1e2e; }
            QTabBar::tab { background: #181825; color: #a6adc8; padding: 10px 20px; border: 1px solid #313244; border-bottom: none; border-top-left-radius: 6px; border-top-right-radius: 6px; margin-right: 2px; }
            QTabBar::tab:selected { background: #1e1e2e; color: #cdd6f4; border-bottom: 1px solid #1e1e2e; }
            QTabBar::tab:hover { background: #313244; }

            QPushButton { background-color: #313244; color: #cdd6f4; border: none; border-radius: 6px; padding: 10px 20px; font-weight: 500; }
            QPushButton:hover { background-color: #45475a; }
            QPushButton:pressed { background-color: #585b70; }
            QPushButton:disabled { background-color: #1e1e2e; color: #6c6f85; }
            QPushButton#PrimaryButton { background-color: #89b4fa; color: #1e1e2e; }
            QPushButton#PrimaryButton:hover { background-color: #74c7ec; }

            QLineEdit, QTextEdit, QComboBox, QSpinBox, QDoubleSpinBox { background-color: #181825; color: #cdd6f4; border: 1px solid #313244; border-radius: 6px; padding: 8px; selection-background-color: #89b4fa; }
            QLineEdit:focus, QTextEdit:focus, QComboBox:focus { border-color: #89b4fa; }
            QComboBox::drop-down { border: none; width: 24px; }
            QComboBox QAbstractItemView { background: #1e1e2e; border: 1px solid #313244; selection-background-color: #313244; }

            QGroupBox { color: #cdd6f4; border: 1px solid #313244; border-radius: 8px; margin-top: 12px; padding-top: 16px; font-weight: bold; }
            QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; color: #89b4fa; }

            QTreeWidget, QListWidget, QTableWidget { background-color: #181825; color: #cdd6f4; border: 1px solid #313244; border-radius: 6px; alternate-background-color: #1e1e2e; show-decoration-selected: 1; }
            QTreeWidget::item, QListWidget::item { padding: 8px; border-radius: 4px; }
            QTreeWidget::item:selected, QListWidget::item:selected { background-color: #313244; color: #cdd6f4; }
            QHeaderView::section { background-color: #181825; color: #a6adc8; border: none; border-right: 1px solid #313244; border-bottom: 1px solid #313244; padding: 8px; font-weight: bold; }

            QScrollBar:vertical { background: #181825; width: 10px; border: none; }
            QScrollBar::handle:vertical { background: #45475a; border-radius: 5px; min-height: 30px; }
            QScrollBar::handle:vertical:hover { background: #585b70; }
            QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }

            QFrame#FeatureCard { background-color: #181825; border: 1px solid #313244; border-radius: 12px; }
            QFrame#FeatureCard:hover { border-color: #89b4fa; }

            QLabel#TitleLabel { color: #cdd6f4; }
            QLabel#SubtitleLabel { color: #a6adc8; }

            QSplitter::handle { background: #313244; }
            QSplitter::handle:horizontal { width: 2px; }
            QSplitter::handle:vertical { height: 2px; }

            QDialog { background-color: #1e1e2e; }
            QCheckBox::indicator { width: 18px; height: 18px; border: 2px solid #313244; border-radius: 4px; background: #181825; }
            QCheckBox::indicator:checked { background-color: #89b4fa; border-color: #89b4fa; image: url(:/icons/check.svg); }
            QRadioButton::indicator { width: 18px; height: 18px; border: 2px solid #313244; border-radius: 9px; background: #181825; }
            QRadioButton::indicator:checked { background-color: #89b4fa; border-color: #89b4fa; }
        )";
    } else {
        styleSheet = R"(
            QMainWindow, QWidget { background-color: #f5f5f5; color: #1e1e2e; }
            QMenuBar { background-color: #ffffff; color: #1e1e2e; border: none; padding: 4px; }
            QMenuBar::item { background: transparent; padding: 6px 12px; border-radius: 4px; }
            QMenuBar::item:selected { background-color: #e8e8e8; }
            QMenu { background-color: #ffffff; border: 1px solid #d0d0d0; border-radius: 6px; padding: 4px; }
            QMenu::item { padding: 8px 24px; border-radius: 4px; }
            QMenu::item:selected { background-color: #e8e8e8; }
            QMenu::separator { height: 1px; background: #d0d0d0; margin: 4px 8px; }

            QToolBar { background-color: #ffffff; border: none; spacing: 8px; padding: 8px; border-bottom: 1px solid #d0d0d0; }
            QToolBar::separator { background: #d0d0d0; width: 1px; margin: 4px; }
            QToolButton { background: transparent; border: none; border-radius: 6px; padding: 8px 12px; color: #1e1e2e; }
            QToolButton:hover { background-color: #e8e8e8; }
            QToolButton:pressed { background-color: #d0d0d0; }

            QStatusBar { background-color: #ffffff; border-top: 1px solid #d0d0d0; color: #666; }
            QProgressBar { border: none; border-radius: 4px; background-color: #e8e8e8; text-align: center; color: #1e1e2e; height: 16px; }
            QProgressBar::chunk { background-color: #3b82f6; border-radius: 4px; }

            QTabWidget::pane { border: 1px solid #d0d0d0; border-radius: 6px; background: #f5f5f5; }
            QTabBar::tab { background: #ffffff; color: #666; padding: 10px 20px; border: 1px solid #d0d0d0; border-bottom: none; border-top-left-radius: 6px; border-top-right-radius: 6px; margin-right: 2px; }
            QTabBar::tab:selected { background: #f5f5f5; color: #1e1e2e; border-bottom: 1px solid #f5f5f5; }
            QTabBar::tab:hover { background: #f0f0f0; }

            QPushButton { background-color: #e8e8e8; color: #1e1e2e; border: none; border-radius: 6px; padding: 10px 20px; font-weight: 500; }
            QPushButton:hover { background-color: #d0d0d0; }
            QPushButton:pressed { background-color: #b8b8b8; }
            QPushButton:disabled { background-color: #f5f5f5; color: #999; }
            QPushButton#PrimaryButton { background-color: #3b82f6; color: #ffffff; }
            QPushButton#PrimaryButton:hover { background-color: #2563eb; }

            QLineEdit, QTextEdit, QComboBox, QSpinBox, QDoubleSpinBox { background-color: #ffffff; color: #1e1e2e; border: 1px solid #d0d0d0; border-radius: 6px; padding: 8px; selection-background-color: #3b82f6; }
            QLineEdit:focus, QTextEdit:focus, QComboBox:focus { border-color: #3b82f6; }
            QComboBox::drop-down { border: none; width: 24px; }
            QComboBox QAbstractItemView { background: #ffffff; border: 1px solid #d0d0d0; selection-background-color: #e8e8e8; }

            QGroupBox { color: #1e1e2e; border: 1px solid #d0d0d0; border-radius: 8px; margin-top: 12px; padding-top: 16px; font-weight: bold; }
            QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; color: #3b82f6; }

            QTreeWidget, QListWidget, QTableWidget { background-color: #ffffff; color: #1e1e2e; border: 1px solid #d0d0d0; border-radius: 6px; alternate-background-color: #f5f5f5; show-decoration-selected: 1; }
            QTreeWidget::item, QListWidget::item { padding: 8px; border-radius: 4px; }
            QTreeWidget::item:selected, QListWidget::item:selected { background-color: #e8e8e8; color: #1e1e2e; }
            QHeaderView::section { background-color: #ffffff; color: #666; border: none; border-right: 1px solid #d0d0d0; border-bottom: 1px solid #d0d0d0; padding: 8px; font-weight: bold; }

            QScrollBar:vertical { background: #f5f5f5; width: 10px; border: none; }
            QScrollBar::handle:vertical { background: #c0c0c0; border-radius: 5px; min-height: 30px; }
            QScrollBar::handle:vertical:hover { background: #a0a0a0; }
            QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }

            QFrame#FeatureCard { background-color: #ffffff; border: 1px solid #d0d0d0; border-radius: 12px; }
            QFrame#FeatureCard:hover { border-color: #3b82f6; }

            QLabel#TitleLabel { color: #1e1e2e; }
            QLabel#SubtitleLabel { color: #666; }

            QSplitter::handle { background: #d0d0d0; }
            QSplitter::handle:horizontal { width: 2px; }
            QSplitter::handle:vertical { height: 2px; }

            QDialog { background-color: #f5f5f5; }
            QCheckBox::indicator { width: 18px; height: 18px; border: 2px solid #d0d0d0; border-radius: 4px; background: #ffffff; }
            QCheckBox::indicator:checked { background-color: #3b82f6; border-color: #3b82f6; image: url(:/icons/check.svg); }
            QRadioButton::indicator { width: 18px; height: 18px; border: 2px solid #d0d0d0; border-radius: 9px; background: #ffffff; }
            QRadioButton::indicator:checked { background-color: #3b82f6; border-color: #3b82f6; }
        )";
    }
    qApp->setStyleSheet(styleSheet);
}

void MainWindow::loadSettings() {
    m_darkTheme = m_settings.value("darkTheme", true).toBool();
    restoreGeometry(m_settings.value("windowGeometry").toByteArray());
    restoreState(m_settings.value("windowState").toByteArray());
}

void MainWindow::saveSettings() {
    m_settings.setValue("darkTheme", m_darkTheme);
    m_settings.setValue("windowGeometry", saveGeometry());
    m_settings.setValue("windowState", saveState());
}

void MainWindow::updateWindowTitle() {
    QString title = "gcanner - Game System Requirements Analyzer";
    if (!m_currentResultPath.isEmpty()) {
        title += " - " + QFileInfo(m_currentResultPath).fileName();
    }
    setWindowTitle(title);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    saveSettings();
    QMainWindow::closeEvent(event);
}

} // namespace gcanner