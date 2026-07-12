#include "ScanWidget.hpp"
#include "ScanWorker.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QDateTime>
#include <QScrollArea>
#include <QSplitter>
#include <QTextEdit>
#include <QProgressBar>
#include <QTimer>
#include <QThread>

namespace gcanner {

ScanWidget::ScanWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &ScanWidget::updateElapsedTime);
}

void ScanWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_stackedWidget = new QStackedWidget();
    mainLayout->addWidget(m_stackedWidget);

    // Page 1: Scan Configuration
    m_scanConfigWidget = new QWidget();
    QVBoxLayout* configLayout = new QVBoxLayout(m_scanConfigWidget);
    configLayout->setSpacing(16);
    configLayout->setContentsMargins(24, 24, 24, 24);

    // Title
    QLabel* titleLabel = new QLabel("New Scan Configuration");
    titleLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #2c3e50; margin-bottom: 8px;");
    configLayout->addWidget(titleLabel);

    QLabel* descLabel = new QLabel("Configure scan parameters and select a game directory to analyze.");
    descLabel->setStyleSheet("font-size: 14px; color: #7f8c8d; margin-bottom: 16px;");
    descLabel->setWordWrap(true);
    configLayout->addWidget(descLabel);

    // Path selection
    QGroupBox* pathGroup = new QGroupBox("Game Directory");
    QVBoxLayout* pathLayout = new QVBoxLayout(pathGroup);
    pathLayout->setSpacing(12);

    QHBoxLayout* pathRow = new QHBoxLayout();
    m_pathEdit = new QLineEdit();
    m_pathEdit->setPlaceholderText("Select game installation directory...");
    m_pathEdit->setMinimumHeight(44);
    m_browseBtn = new QPushButton("Browse...");
    m_browseBtn->setMinimumSize(100, 44);
    m_browseBtn->setCursor(Qt::PointingHandCursor);
    connect(m_browseBtn, &QPushButton::clicked, this, &ScanWidget::onBrowseClicked);
    pathRow->addWidget(m_pathEdit, 1);
    pathRow->addWidget(m_browseBtn);
    pathLayout->addLayout(pathRow);

    configLayout->addWidget(pathGroup);

    // Scan options
    QWidget* optionsWidget = new QWidget();
    QHBoxLayout* optionsLayout = new QHBoxLayout(optionsWidget);
    optionsLayout->setSpacing(16);

    // Left column - Scan settings
    QGroupBox* scanGroup = new QGroupBox("Scan Settings");
    QFormLayout* scanLayout = new QFormLayout(scanGroup);
    scanLayout->setSpacing(12);
    scanLayout->setLabelAlignment(Qt::AlignLeft);

    m_recursiveCheck = new QCheckBox("Recursive scan");
    m_recursiveCheck->setChecked(true);
    scanLayout->addRow(m_recursiveCheck);

    m_followSymlinksCheck = new QCheckBox("Follow symbolic links");
    m_followSymlinksCheck->setChecked(false);
    scanLayout->addRow(m_followSymlinksCheck);

    m_maxDepthSpin = new QSpinBox();
    m_maxDepthSpin->setRange(1, 100);
    m_maxDepthSpin->setValue(20);
    m_maxDepthSpin->setSuffix(" levels");
    scanLayout->addRow("Max depth:", m_maxDepthSpin);

    m_threadCountSpin = new QSpinBox();
    m_threadCountSpin->setRange(1, QThread::idealThreadCount());
    m_threadCountSpin->setValue(QThread::idealThreadCount());
    m_threadCountSpin->setSuffix(" threads");
    scanLayout->addRow("Threads:", m_threadCountSpin);

    m_verifyChecksumsCheck = new QCheckBox("Verify file checksums");
    m_verifyChecksumsCheck->setChecked(false);
    scanLayout->addRow(m_verifyChecksumsCheck);

    m_scanTypeCombo = new QComboBox();
    m_scanTypeCombo->addItems({"Full Scan", "Quick Scan", "Custom Scan"});
    connect(m_scanTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ScanWidget::onScanTypeChanged);
    scanLayout->addRow("Scan type:", m_scanTypeCombo);

    optionsLayout->addWidget(scanGroup, 1);

    // Right column - Analyzers
    m_analyzersGroup = new QGroupBox("Asset Analyzers");
    QVBoxLayout* analyzersLayout = new QVBoxLayout(m_analyzersGroup);
    analyzersLayout->setSpacing(8);

    auto createAnalyzerCheck = [this, &analyzersLayout](const QString& text, bool defaultOn = true) {
        QCheckBox* cb = new QCheckBox(text);
        cb->setChecked(defaultOn);
        analyzersLayout->addWidget(cb);
        return cb;
    };

    m_texturesCheck = createAnalyzerCheck("🖼️ Textures (DDS, KTX, ASTC, PNG, JPEG, etc.)");
    m_modelsCheck = createAnalyzerCheck("📦 3D Models (FBX, OBJ, GLTF, COLLADA, etc.)");
    m_audioCheck = createAnalyzerCheck("🔊 Audio (WAV, OGG, MP3, FLAC, XMA, etc.)");
    m_scriptsCheck = createAnalyzerCheck("📜 Scripts (C#, Lua, Python, JS, HLSL/GLSL, etc.)");
    m_executablesCheck = createAnalyzerCheck("⚙️ Executables (PE, ELF, Mach-O)");
    m_shadersCheck = createAnalyzerCheck("🎨 Shaders (HLSL, GLSL, SPIR-V, MSL, DXBC, DXIL)");
    m_particlesCheck = createAnalyzerCheck("✨ Particles (PFX, PTX, PAR, EMITTER)");
    m_physicsCheck = createAnalyzerCheck("⚡ Physics (PhysX, Havok, Bullet, ODE, Newton)");
    m_aiCheck = createAnalyzerCheck("🧠 AI (NavMesh, Behavior Trees, State Machines, Neural Nets)");
    m_networkCheck = createAnalyzerCheck("🌐 Network (Photon, Mirror, Steam, EOS, RakNet, ENet)");

    QPushButton* selectAllBtn = new QPushButton("Select All");
    selectAllBtn->setCursor(Qt::PointingHandCursor);
    selectAllBtn->setStyleSheet("border: none; color: #89b4fa; padding: 4px;");
    connect(selectAllBtn, &QPushButton::clicked, [this](){
        m_texturesCheck->setChecked(true);
        m_modelsCheck->setChecked(true);
        m_audioCheck->setChecked(true);
        m_scriptsCheck->setChecked(true);
        m_executablesCheck->setChecked(true);
        m_shadersCheck->setChecked(true);
        m_particlesCheck->setChecked(true);
        m_physicsCheck->setChecked(true);
        m_aiCheck->setChecked(true);
        m_networkCheck->setChecked(true);
    });
    analyzersLayout->addWidget(selectAllBtn, 0, Qt::AlignLeft);

    QPushButton* selectNoneBtn = new QPushButton("Select None");
    selectNoneBtn->setCursor(Qt::PointingHandCursor);
    selectNoneBtn->setStyleSheet("border: none; color: #89b4fa; padding: 4px;");
    connect(selectNoneBtn, &QPushButton::clicked, [this](){
        m_texturesCheck->setChecked(false);
        m_modelsCheck->setChecked(false);
        m_audioCheck->setChecked(false);
        m_scriptsCheck->setChecked(false);
        m_executablesCheck->setChecked(false);
        m_shadersCheck->setChecked(false);
        m_particlesCheck->setChecked(false);
        m_physicsCheck->setChecked(false);
        m_aiCheck->setChecked(false);
        m_networkCheck->setChecked(false);
    });
    analyzersLayout->addWidget(selectNoneBtn, 0, Qt::AlignLeft);

    optionsLayout->addWidget(m_analyzersGroup, 1);
    configLayout->addWidget(optionsWidget);

    // Output options
    m_outputGroup = new QGroupBox("Output Options");
    QFormLayout* outputLayout = new QFormLayout(m_outputGroup);
    outputLayout->setSpacing(12);

    m_outputFormatCombo = new QComboBox();
    m_outputFormatCombo->addItems({"JSON", "CSV", "HTML", "Markdown", "All Formats"});
    outputLayout->addRow("Format:", m_outputFormatCombo);

    QHBoxLayout* outputPathLayout = new QHBoxLayout();
    m_outputPathEdit = new QLineEdit();
    m_outputPathEdit->setPlaceholderText("Auto: same directory as game");
    m_outputPathEdit->setMinimumHeight(44);
    m_outputBrowseBtn = new QPushButton("Browse...");
    m_outputBrowseBtn->setMinimumSize(100, 44);
    m_outputBrowseBtn->setCursor(Qt::PointingHandCursor);
    connect(m_outputBrowseBtn, &QPushButton::clicked, [this](){
        QString dir = QFileDialog::getExistingDirectory(this, "Select Output Directory");
        if (!dir.isEmpty()) m_outputPathEdit->setText(dir);
    });
    outputPathLayout->addWidget(m_outputPathEdit, 1);
    outputPathLayout->addWidget(m_outputBrowseBtn);
    outputLayout->addRow("Output Dir:", outputPathLayout);

    m_openAfterScanCheck = new QCheckBox("Open results after scan");
    m_openAfterScanCheck->setChecked(true);
    outputLayout->addRow(m_openAfterScanCheck);

    configLayout->addWidget(m_outputGroup);

    // Start button
    QPushButton* startBtn = new QPushButton("Start Scan");
    startBtn->setObjectName("PrimaryButton");
    startBtn->setMinimumHeight(50);
    startBtn->setCursor(Qt::PointingHandCursor);
    startBtn->setStyleSheet("font-size: 16px; font-weight: bold;");
    connect(startBtn, &QPushButton::clicked, this, &ScanWidget::startNewScan);
    configLayout->addWidget(startBtn);

    configLayout->addStretch();

    // Page 2: Scan Progress
    m_scanProgressWidget = new QWidget();
    QVBoxLayout* progressLayout = new QVBoxLayout(m_scanProgressWidget);
    progressLayout->setSpacing(16);
    progressLayout->setContentsMargins(24, 24, 24, 24);
    progressLayout->setAlignment(Qt::AlignTop);

    QLabel* progressTitle = new QLabel("Scanning in Progress");
    progressTitle->setStyleSheet("font-size: 24px; font-weight: bold; color: #2c3e50;");
    progressLayout->addWidget(progressTitle);

    // Overall progress
    QGroupBox* overallGroup = new QGroupBox("Overall Progress");
    QVBoxLayout* overallLayout = new QVBoxLayout(overallGroup);
    m_mainProgress = new QProgressBar();
    m_mainProgress->setMinimumHeight(24);
    m_mainProgress->setTextVisible(true);
    m_mainProgress->setFormat("Overall: %p% (%v of %m files)");
    overallLayout->addWidget(m_mainProgress);

    QHBoxLayout* statsLayout = new QHBoxLayout();
    m_filesProcessedLabel = new QLabel("Files: 0 / 0");
    m_elapsedTimeLabel = new QLabel("Elapsed: 00:00");
    m_etaLabel = new QLabel("ETA: --:--");
    statsLayout->addWidget(m_filesProcessedLabel);
    statsLayout->addStretch();
    statsLayout->addWidget(m_elapsedTimeLabel);
    statsLayout->addStretch();
    statsLayout->addWidget(m_etaLabel);
    overallLayout->addLayout(statsLayout);
    progressLayout->addWidget(overallGroup);

    // Current file
    QGroupBox* fileGroup = new QGroupBox("Current File");
    QVBoxLayout* fileLayout = new QVBoxLayout(fileGroup);
    m_currentFileLabel = new QLabel("Waiting to start...");
    m_currentFileLabel->setWordWrap(true);
    m_currentFileLabel->setStyleSheet("font-family: monospace; font-size: 13px; color: #89b4fa; padding: 8px; background: #181825; border-radius: 4px;");
    fileLayout->addWidget(m_currentFileLabel);
    m_fileProgress = new QProgressBar();
    m_fileProgress->setMinimumHeight(8);
    m_fileProgress->setTextVisible(false);
    fileLayout->addWidget(m_fileProgress);
    progressLayout->addWidget(fileGroup);

    // Log output
    QGroupBox* logGroup = new QGroupBox("Scan Log");
    QVBoxLayout* logLayout = new QVBoxLayout(logGroup);
    m_logOutput = new QTextEdit();
    m_logOutput->setReadOnly(true);
    m_logOutput->setFont(QFont("Monospace", 11));
    m_logOutput->setMinimumHeight(200);
    m_logOutput->setStyleSheet("background: #181825; border: 1px solid #313244; border-radius: 6px; color: #cdd6f4;");
    logLayout->addWidget(m_logOutput);
    progressLayout->addWidget(logGroup, 1);

    // Control buttons
    QHBoxLayout* btnLayout = new QHBoxLayout();
    m_stopBtn = new QPushButton("Stop Scan");
    m_stopBtn->setMinimumHeight(44);
    m_stopBtn->setCursor(Qt::PointingHandCursor);
    connect(m_stopBtn, &QPushButton::clicked, this, &ScanWidget::stopScan);
    btnLayout->addStretch();
    btnLayout->addWidget(m_stopBtn);

    m_viewResultsBtn = new QPushButton("View Results");
    m_viewResultsBtn->setObjectName("PrimaryButton");
    m_viewResultsBtn->setMinimumSize(140, 44);
    m_viewResultsBtn->setVisible(false);
    m_viewResultsBtn->setCursor(Qt::PointingHandCursor);
    connect(m_viewResultsBtn, &QPushButton::clicked, [this](){
        emit scanFinished(m_worker ? m_worker->resultPath() : QString());
    });
    btnLayout->addWidget(m_viewResultsBtn);
    progressLayout->addLayout(btnLayout);

    // Add pages to stack
    m_stackedWidget->addWidget(m_scanConfigWidget);
    m_stackedWidget->addWidget(m_scanProgressWidget);
}

void ScanWidget::startNewScan() {
    QString path = getSelectedPath();
    if (path.isEmpty()) {
        QMessageBox::warning(this, "No Directory", "Please select a game directory to scan.");
        return;
    }

    if (!QDir(path).exists()) {
        QMessageBox::warning(this, "Invalid Directory", "The selected directory does not exist.");
        return;
    }

    // Collect enabled analyzers
    QStringList analyzers;
    if (m_texturesCheck->isChecked()) analyzers << "textures";
    if (m_modelsCheck->isChecked()) analyzers << "models";
    if (m_audioCheck->isChecked()) analyzers << "audio";
    if (m_scriptsCheck->isChecked()) analyzers << "scripts";
    if (m_executablesCheck->isChecked()) analyzers << "executables";
    if (m_shadersCheck->isChecked()) analyzers << "shaders";
    if (m_particlesCheck->isChecked()) analyzers << "particles";
    if (m_physicsCheck->isChecked()) analyzers << "physics";
    if (m_aiCheck->isChecked()) analyzers << "ai";
    if (m_networkCheck->isChecked()) analyzers << "network";

    if (analyzers.isEmpty()) {
        QMessageBox::warning(this, "No Analyzers", "Please enable at least one asset analyzer.");
        return;
    }

    // Switch to progress page
    m_stackedWidget->setCurrentWidget(m_scanProgressWidget);
    resetUI();

    // Create worker
    m_worker = new ScanWorker();
    m_worker->setPath(path);
    m_worker->setAnalyzers(analyzers);
    m_worker->setRecursive(m_recursiveCheck->isChecked());
    m_worker->setFollowSymlinks(m_followSymlinksCheck->isChecked());
    m_worker->setMaxDepth(m_maxDepthSpin->value());
    m_worker->setThreadCount(m_threadCountSpin->value());
    m_worker->setVerifyChecksums(m_verifyChecksumsCheck->isChecked());
    m_worker->setOutputFormat(m_outputFormatCombo->currentText().toLower());
    m_worker->setOutputDir(m_outputPathEdit->text());

    m_workerThread = new QThread(this);
    m_worker->moveToThread(m_workerThread);

    connect(m_workerThread, &QThread::started, m_worker, &ScanWorker::process);
    connect(m_worker, &ScanWorker::progress, this, &ScanWidget::onWorkerProgress);
    connect(m_worker, &ScanWorker::finished, this, &ScanWidget::onWorkerFinished);
    connect(m_worker, &ScanWorker::error, this, &ScanWidget::onWorkerError);
    connect(m_worker, &ScanWorker::finished, m_workerThread, &QThread::quit);
    connect(m_worker, &ScanWorker::finished, m_worker, &QObject::deleteLater);
    connect(m_workerThread, &QThread::finished, m_workerThread, &QObject::deleteLater);

    emit scanStarted();
    m_workerThread->start();
    m_timer->start(1000);
    m_startTime.start();
}

void ScanWidget::stopScan() {
    if (m_worker) {
        m_worker->requestStop();
    }
    m_stopBtn->setEnabled(false);
    m_stopBtn->setText("Stopping...");
}

void ScanWidget::onBrowseClicked() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Game Directory", QDir::homePath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty()) {
        m_pathEdit->setText(dir);
    }
}

void ScanWidget::onScanTypeChanged(int index) {
    bool custom = (index == 2);
    m_recursiveCheck->setEnabled(custom);
    m_followSymlinksCheck->setEnabled(custom);
    m_maxDepthSpin->setEnabled(custom);
    m_threadCountSpin->setEnabled(custom);
    m_verifyChecksumsCheck->setEnabled(custom);
}

void ScanWidget::onWorkerProgress(int progress, const QString& status) {
    m_mainProgress->setValue(progress);
    m_currentFileLabel->setText(status);
    m_logOutput->append(QString("[%1] %2").arg(QTime::currentTime().toString("HH:mm:ss"), status));
    m_logOutput->verticalScrollBar()->setValue(m_logOutput->verticalScrollBar()->maximum());

    // Update file progress (simulated)
    static int fileProgress = 0;
    fileProgress = (fileProgress + 10) % 100;
    m_fileProgress->setValue(fileProgress);
}

void ScanWidget::onWorkerFinished(const QString& resultPath) {
    m_timer->stop();
    m_mainProgress->setValue(100);
    m_fileProgress->setValue(100);
    m_currentFileLabel->setText("Scan complete!");
    m_statusLabel->setText("Scan complete");
    m_stopBtn->setVisible(false);
    m_viewResultsBtn->setVisible(true);

    emit scanFinished(resultPath);
}

void ScanWidget::onWorkerError(const QString& error) {
    m_timer->stop();
    m_stopBtn->setVisible(false);
    m_logOutput->append(QString("<span style='color: #f38ba8;'>[ERROR] %1</span>").arg(error));
    emit scanError(error);
}

void ScanWidget::updateElapsedTime() {
    int elapsed = m_startTime.elapsed() / 1000;
    int hours = elapsed / 3600;
    int minutes = (elapsed % 3600) / 60;
    int seconds = elapsed % 60;
    m_elapsedTimeLabel->setText(QString("Elapsed: %1:%2:%3").arg(hours, 2, 10, QChar('0')).arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0')));

    if (m_mainProgress->maximum() > 0 && m_mainProgress->value() > 0) {
        int total = m_mainProgress->maximum();
        int current = m_mainProgress->value();
        if (current > 0) {
            int remaining = (elapsed * total / current) - elapsed;
            int rHours = remaining / 3600;
            int rMinutes = (remaining % 3600) / 60;
            int rSeconds = remaining % 60;
            m_etaLabel->setText(QString("ETA: %1:%2:%3").arg(rHours, 2, 10, QChar('0')).arg(rMinutes, 2, 10, QChar('0')).arg(rSeconds, 2, 10, QChar('0')));
        }
    }
}

void ScanWidget::resetUI() {
    m_mainProgress->setValue(0);
    m_mainProgress->setMaximum(100);
    m_fileProgress->setValue(0);
    m_currentFileLabel->setText("Initializing scan...");
    m_filesProcessedLabel->setText("Files: 0 / 0");
    m_elapsedTimeLabel->setText("Elapsed: 00:00");
    m_etaLabel->setText("ETA: --:--");
    m_logOutput->clear();
    m_stopBtn->setVisible(true);
    m_stopBtn->setEnabled(true);
    m_stopBtn->setText("Stop Scan");
    m_viewResultsBtn->setVisible(false);
    m_startTime.start();
}

QString ScanWidget::getSelectedPath() const {
    return m_pathEdit->text().trimmed();
}

} // namespace gcanner