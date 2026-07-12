#include "SettingsDialog.hpp"

#include <QVBoxLayout>
#include <QTabWidget>
#include <QGroupBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>

namespace gcanner {

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Settings");
    setModal(true);
    resize(600, 500);
    setMinimumSize(550, 450);

    setupUI();
    loadSettings();

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::onAccepted);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::onRejected);
    connect(m_resetBtn, &QPushButton::clicked, this, &SettingsDialog::onResetDefaults);
    connect(m_exportBtn, &QPushButton::clicked, this, &SettingsDialog::onExportSettings);
    connect(m_importBtn, &QPushButton::clicked, this, &SettingsDialog::onImportSettings);
}

SettingsDialog::~SettingsDialog() = default;

void SettingsDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    m_tabWidget = new QTabWidget();
    m_tabWidget->setDocumentMode(true);
    mainLayout->addWidget(m_tabWidget, 1);

    setupGeneralTab();
    setupScanningTab();
    setupAnalyzersTab();
    setupOutputTab();
    setupAdvancedTab();

    // Button box
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
    connect(m_buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &SettingsDialog::onAccepted);
    mainLayout->addWidget(m_buttonBox);
}

void SettingsDialog::setupGeneralTab() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);
    layout->setSpacing(16);
    layout->setContentsMargins(16, 16, 16, 16);

    // Appearance
    QGroupBox* appearanceGroup = new QGroupBox("Appearance");
    QFormLayout* appearanceLayout = new QFormLayout(appearanceGroup);

    m_darkThemeCheck = new QCheckBox("Dark theme");
    m_darkThemeCheck->setToolTip("Use dark color scheme (requires restart)");
    appearanceLayout->addRow(m_darkThemeCheck);

    m_languageCombo = new QComboBox();
    m_languageCombo->addItems({"English", "Spanish", "French", "German", "Chinese", "Japanese", "Korean"});
    appearanceLayout->addRow("Language:", m_languageCombo);

    m_startupViewCombo = new QComboBox();
    m_startupViewCombo->addItems({"Welcome Page", "Scan View", "Last Used View"});
    appearanceLayout->addRow("Startup view:", m_startupViewCombo);

    layout->addWidget(appearanceGroup);

    // Behavior
    QGroupBox* behaviorGroup = new QGroupBox("Behavior");
    QFormLayout* behaviorLayout = new QFormLayout(behaviorGroup);

    m_startMinimizedCheck = new QCheckBox("Start minimized to tray");
    behaviorLayout->addRow(m_startMinimizedCheck);

    m_checkUpdatesCheck = new QCheckBox("Check for updates on startup");
    m_checkUpdatesCheck->setChecked(true);
    behaviorLayout->addRow(m_checkUpdatesCheck);

    layout->addWidget(behaviorGroup);
    layout->addStretch();

    m_tabWidget->addTab(tab, "General");
}

void SettingsDialog::setupScanningTab() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);
    layout->setSpacing(16);
    layout->setContentsMargins(16, 16, 16, 16);

    QGroupBox* defaultsGroup = new QGroupBox("Default Scan Options");
    QFormLayout* defaultsLayout = new QFormLayout(defaultsGroup);

    m_recursiveDefaultCheck = new QCheckBox();
    m_recursiveDefaultCheck->setChecked(true);
    defaultsLayout->addRow("Recursive by default:", m_recursiveDefaultCheck);

    m_maxDepthDefaultSpin = new QSpinBox();
    m_maxDepthDefaultSpin->setRange(1, 100);
    m_maxDepthDefaultSpin->setValue(20);
    m_maxDepthDefaultSpin->setSuffix(" levels");
    defaultsLayout->addRow("Max depth:", m_maxDepthDefaultSpin);

    m_threadCountDefaultSpin = new QSpinBox();
    m_threadCountDefaultSpin->setRange(1, QThread::idealThreadCount());
    m_threadCountDefaultSpin->setValue(QThread::idealThreadCount());
    m_threadCountDefaultSpin->setSuffix(" threads");
    defaultsLayout->addRow("Thread count:", m_threadCountDefaultSpin);

    m_verifyChecksumsDefaultCheck = new QCheckBox();
    defaultsLayout->addRow("Verify checksums:", m_verifyChecksumsDefaultCheck);

    m_useMmapCheck = new QCheckBox("Use memory-mapped files for large files");
    m_useMmapCheck->setChecked(true);
    defaultsLayout->addRow(m_useMmapCheck);

    layout->addWidget(defaultsGroup);

    // Cache & Data
    QGroupBox* dirsGroup = new QGroupBox("Directories");
    QFormLayout* dirsLayout = new QFormLayout(dirsGroup);

    m_cacheDirEdit = new QLineEdit();
    m_cacheDirEdit->setPlaceholderText("Cache directory (auto if empty)");
    QPushButton* cacheBrowse = new QPushButton("Browse...");
    connect(cacheBrowse, &QPushButton::clicked, this, &SettingsDialog::onBrowseCacheDir);
    QHBoxLayout* cacheLayout = new QHBoxLayout();
    cacheLayout->addWidget(m_cacheDirEdit, 1);
    cacheLayout->addWidget(cacheBrowse);
    dirsLayout->addRow("Cache directory:", cacheLayout);

    m_dataDirEdit = new QLineEdit();
    m_dataDirEdit->setPlaceholderText("Hardware data directory (auto if empty)");
    QPushButton* dataBrowse = new QPushButton("Browse...");
    connect(dataBrowse, &QPushButton::clicked, this, &SettingsDialog::onBrowseDataDir);
    QHBoxLayout* dataLayout = new QHBoxLayout();
    dataLayout->addWidget(m_dataDirEdit, 1);
    dataLayout->addWidget(dataBrowse);
    dirsLayout->addRow("Data directory:", dataLayout);

    layout->addWidget(dirsGroup);
    layout->addStretch();

    m_tabWidget->addTab(tab, "Scanning");
}

void SettingsDialog::setupAnalyzersTab() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);
    layout->setSpacing(8);
    layout->setContentsMargins(16, 16, 16, 16);

    QLabel* infoLabel = new QLabel("Enable or disable individual asset analyzers. Disabled analyzers will be skipped during scans.");
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("color: #a6adc8; font-size: 13px; padding: 8px; background: #181825; border-radius: 6px;");
    layout->addWidget(infoLabel);

    auto createCheck = [this, &layout](const QString& name, const QString& tooltip, bool defaultOn = true) {
        QCheckBox* cb = new QCheckBox(name);
        cb->setToolTip(tooltip);
        cb->setChecked(defaultOn);
        layout->addWidget(cb);
        return cb;
    };

    m_texturesEnabledCheck = createCheck("🖼️ Textures (DDS, KTX, ASTC, PNG, JPEG, etc.)", "Analyze texture files for resolution, format, compression");
    m_modelsEnabledCheck = createCheck("📦 3D Models (FBX, OBJ, GLTF, COLLADA, etc.)", "Analyze model files for vertex count, materials, animations");
    m_audioEnabledCheck = createCheck("🔊 Audio (WAV, OGG, MP3, FLAC, XMA, etc.)", "Analyze audio files for duration, bitrate, channels");
    m_scriptsEnabledCheck = createCheck("📜 Scripts (C#, Lua, Python, JS, HLSL/GLSL, etc.)", "Analyze script files for complexity, dependencies");
    m_executablesEnabledCheck = createCheck("⚙️ Executables (PE, ELF, Mach-O)", "Analyze binary executables for engine detection, dependencies");
    m_shadersEnabledCheck = createCheck("🎨 Shaders (HLSL, GLSL, SPIR-V, MSL, DXBC, DXIL)", "Analyze shader code for complexity, resource usage");
    m_particlesEnabledCheck = createCheck("✨ Particles (PFX, PTX, PAR, EMITTER)", "Analyze particle system files for emitter count, complexity");
    m_physicsEnabledCheck = createCheck("⚡ Physics (PhysX, Havok, Bullet, ODE, Newton)", "Analyze physics assets for collision shapes, constraints");
    m_aiEnabledCheck = createCheck("🧠 AI (NavMesh, Behavior Trees, State Machines, Neural Nets)", "Analyze AI assets for complexity, node counts");
    m_networkEnabledCheck = createCheck("🌐 Network (Photon, Mirror, Steam, EOS, RakNet, ENet)", "Analyze network libraries and protocols");

    layout->addStretch();

    m_tabWidget->addTab(tab, "Analyzers");
}

void SettingsDialog::setupOutputTab() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);
    layout->setSpacing(16);
    layout->setContentsMargins(16, 16, 16, 16);

    QGroupBox* formatGroup = new QGroupBox("Default Output Format");
    QFormLayout* formatLayout = new QFormLayout(formatGroup);

    m_defaultFormatCombo = new QComboBox();
    m_defaultFormatCombo->addItems({"JSON", "CSV", "HTML", "Markdown", "All"});
    formatLayout->addRow("Format:", m_defaultFormatCombo);

    m_openAfterScanCheck = new QCheckBox("Open results automatically after scan");
    m_openAfterScanCheck->setChecked(true);
    formatLayout->addRow(m_openAfterScanCheck);

    m_showProgressCheck = new QCheckBox("Show detailed progress during scan");
    m_showProgressCheck->setChecked(true);
    formatLayout->addRow(m_showProgressCheck);

    m_showDetailedLogCheck = new QCheckBox("Show detailed log output");
    formatLayout->addRow(m_showDetailedLogCheck);

    m_maxDisplayItemsSpin = new QSpinBox();
    m_maxDisplayItemsSpin->setRange(10, 1000);
    m_maxDisplayItemsSpin->setValue(20);
    m_maxDisplayItemsSpin->setSuffix(" items");
    formatLayout->addRow("Max items in tables:", m_maxDisplayItemsSpin);

    layout->addWidget(formatGroup);
    layout->addStretch();

    m_tabWidget->addTab(tab, "Output");
}

void SettingsDialog::setupAdvancedTab() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);
    layout->setSpacing(16);
    layout->setContentsMargins(16, 16, 16, 16);

    QGroupBox* telemetryGroup = new QGroupBox("Privacy & Telemetry");
    QFormLayout* telemetryLayout = new QFormLayout(telemetryGroup);

    m_enableTelemetryCheck = new QCheckBox("Send anonymous usage statistics");
    m_enableTelemetryCheck->setToolTip("Helps improve gcanner by sending anonymous scan statistics");
    telemetryLayout->addRow(m_enableTelemetryCheck);

    m_enableCrashReportingCheck = new QCheckBox("Automatically report crashes");
    m_enableCrashReportingCheck->setChecked(true);
    m_enableCrashReportingCheck->setToolTip("Send crash reports to help fix bugs");
    telemetryLayout->addRow(m_enableCrashReportingCheck);

    m_logLevelSpin = new QSpinBox();
    m_logLevelSpin->setRange(0, 4);
    m_logLevelSpin->setValue(2);
    m_logLevelSpin->setSuffix(" (0=Trace, 4=Off)");
    telemetryLayout->addRow("Log verbosity:", m_logLevelSpin);

    layout->addWidget(telemetryGroup);

    QGroupBox* customGroup = new QGroupBox("Advanced");
    QFormLayout* customLayout = new QFormLayout(customGroup);

    m_customArgsEdit = new QLineEdit();
    m_customArgsEdit->setPlaceholderText("Additional command-line arguments for scanner");
    customLayout->addRow("Custom args:", m_customArgsEdit);

    layout->addWidget(customGroup);

    // Reset/Export/Import
    QGroupBox* mgmtGroup = new QGroupBox("Settings Management");
    QHBoxLayout* mgmtLayout = new QHBoxLayout(mgmtGroup);

    m_resetBtn = new QPushButton("Reset to Defaults");
    m_resetBtn->setToolTip("Reset all settings to default values");
    m_exportBtn = new QPushButton("Export Settings...");
    m_exportBtn->setToolTip("Save settings to a file");
    m_importBtn = new QPushButton("Import Settings...");
    m_importBtn->setToolTip("Load settings from a file");

    mgmtLayout->addWidget(m_resetBtn);
    mgmtLayout->addWidget(m_exportBtn);
    mgmtLayout->addWidget(m_importBtn);
    mgmtLayout->addStretch();

    layout->addWidget(mgmtGroup);
    layout->addStretch();

    m_tabWidget->addTab(tab, "Advanced");
}

void SettingsDialog::loadSettings() {
    m_darkThemeCheck->setChecked(m_settings.value("darkTheme", true).toBool());
    m_languageCombo->setCurrentText(m_settings.value("language", "English").toString());
    m_startupViewCombo->setCurrentText(m_settings.value("startupView", "Welcome Page").toString());
    m_startMinimizedCheck->setChecked(m_settings.value("startMinimized", false).toBool());
    m_checkUpdatesCheck->setChecked(m_settings.value("checkUpdates", true).toBool());

    m_recursiveDefaultCheck->setChecked(m_settings.value("recursiveDefault", true).toBool());
    m_maxDepthDefaultSpin->setValue(m_settings.value("maxDepthDefault", 20).toInt());
    m_threadCountDefaultSpin->setValue(m_settings.value("threadCountDefault", QThread::idealThreadCount()).toInt());
    m_verifyChecksumsDefaultCheck->setChecked(m_settings.value("verifyChecksumsDefault", false).toBool());
    m_useMmapCheck->setChecked(m_settings.value("useMmap", true).toBool());
    m_cacheDirEdit->setText(m_settings.value("cacheDir").toString());
    m_dataDirEdit->setText(m_settings.value("dataDir").toString());

    m_texturesEnabledCheck->setChecked(m_settings.value("analyzer/textures", true).toBool());
    m_modelsEnabledCheck->setChecked(m_settings.value("analyzer/models", true).toBool());
    m_audioEnabledCheck->setChecked(m_settings.value("analyzer/audio", true).toBool());
    m_scriptsEnabledCheck->setChecked(m_settings.value("analyzer/scripts", true).toBool());
    m_executablesEnabledCheck->setChecked(m_settings.value("analyzer/executables", true).toBool());
    m_shadersEnabledCheck->setChecked(m_settings.value("analyzer/shaders", true).toBool());
    m_particlesEnabledCheck->setChecked(m_settings.value("analyzer/particles", true).toBool());
    m_physicsEnabledCheck->setChecked(m_settings.value("analyzer/physics", true).toBool());
    m_aiEnabledCheck->setChecked(m_settings.value("analyzer/ai", true).toBool());
    m_networkEnabledCheck->setChecked(m_settings.value("analyzer/network", true).toBool());

    m_defaultFormatCombo->setCurrentText(m_settings.value("defaultFormat", "JSON").toString());
    m_openAfterScanCheck->setChecked(m_settings.value("openAfterScan", true).toBool());
    m_showProgressCheck->setChecked(m_settings.value("showProgress", true).toBool());
    m_showDetailedLogCheck->setChecked(m_settings.value("showDetailedLog", false).toBool());
    m_maxDisplayItemsSpin->setValue(m_settings.value("maxDisplayItems", 20).toInt());

    m_enableTelemetryCheck->setChecked(m_settings.value("enableTelemetry", false).toBool());
    m_enableCrashReportingCheck->setChecked(m_settings.value("enableCrashReporting", true).toBool());
    m_logLevelSpin->setValue(m_settings.value("logLevel", 2).toInt());
    m_customArgsEdit->setText(m_settings.value("customArgs").toString());
}

void SettingsDialog::saveSettings() {
    m_settings.setValue("darkTheme", m_darkThemeCheck->isChecked());
    m_settings.setValue("language", m_languageCombo->currentText());
    m_settings.setValue("startupView", m_startupViewCombo->currentText());
    m_settings.setValue("startMinimized", m_startMinimizedCheck->isChecked());
    m_settings.setValue("checkUpdates", m_checkUpdatesCheck->isChecked());

    m_settings.setValue("recursiveDefault", m_recursiveDefaultCheck->isChecked());
    m_settings.setValue("maxDepthDefault", m_maxDepthDefaultSpin->value());
    m_settings.setValue("threadCountDefault", m_threadCountDefaultSpin->value());
    m_settings.setValue("verifyChecksumsDefault", m_verifyChecksumsDefaultCheck->isChecked());
    m_settings.setValue("useMmap", m_useMmapCheck->isChecked());
    m_settings.setValue("cacheDir", m_cacheDirEdit->text());
    m_settings.setValue("dataDir", m_dataDirEdit->text());

    m_settings.setValue("analyzer/textures", m_texturesEnabledCheck->isChecked());
    m_settings.setValue("analyzer/models", m_modelsEnabledCheck->isChecked());
    m_settings.setValue("analyzer/audio", m_audioEnabledCheck->isChecked());
    m_settings.setValue("analyzer/scripts", m_scriptsEnabledCheck->isChecked());
    m_settings.setValue("analyzer/executables", m_executablesEnabledCheck->isChecked());
    m_settings.setValue("analyzer/shaders", m_shadersEnabledCheck->isChecked());
    m_settings.setValue("analyzer/particles", m_particlesEnabledCheck->isChecked());
    m_settings.setValue("analyzer/physics", m_physicsEnabledCheck->isChecked());
    m_settings.setValue("analyzer/ai", m_aiEnabledCheck->isChecked());
    m_settings.setValue("analyzer/network", m_networkEnabledCheck->isChecked());

    m_settings.setValue("defaultFormat", m_defaultFormatCombo->currentText());
    m_settings.setValue("openAfterScan", m_openAfterScanCheck->isChecked());
    m_settings.setValue("showProgress", m_showProgressCheck->isChecked());
    m_settings.setValue("showDetailedLog", m_showDetailedLogCheck->isChecked());
    m_settings.setValue("maxDisplayItems", m_maxDisplayItemsSpin->value());

    m_settings.setValue("enableTelemetry", m_enableTelemetryCheck->isChecked());
    m_settings.setValue("enableCrashReporting", m_enableCrashReportingCheck->isChecked());
    m_settings.setValue("logLevel", m_logLevelSpin->value());
    m_settings.setValue("customArgs", m_customArgsEdit->text());
}

void SettingsDialog::applySettings() {
    saveSettings();
    // Apply theme change immediately if changed
    // ThemeManager::instance().setTheme(m_darkThemeCheck->isChecked() ? ThemeManager::Dark : ThemeManager::Light);
}

void SettingsDialog::onAccepted() {
    applySettings();
    accept();
}

void SettingsDialog::onRejected() {
    reject();
}

void SettingsDialog::onBrowseCacheDir() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Cache Directory", m_cacheDirEdit->text());
    if (!dir.isEmpty()) m_cacheDirEdit->setText(dir);
}

void SettingsDialog::onBrowseDataDir() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Data Directory", m_dataDirEdit->text());
    if (!dir.isEmpty()) m_dataDirEdit->setText(dir);
}

void SettingsDialog::onResetDefaults() {
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Reset Settings",
        "Are you sure you want to reset all settings to default values?",
        QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        m_settings.clear();
        loadSettings();
    }
}

void SettingsDialog::onExportSettings() {
    QString file = QFileDialog::getSaveFileName(this, "Export Settings", QDir::homePath() + "/gcanner_settings.json",
        "JSON Files (*.json)");
    if (file.isEmpty()) return;

    QJsonObject obj;
    obj["darkTheme"] = m_darkThemeCheck->isChecked();
    obj["language"] = m_languageCombo->currentText();
    // ... export all settings
    QJsonDocument doc(obj);
    QFile f(file);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(doc.toJson(QJsonDocument::Indented));
        QMessageBox::information(this, "Success", "Settings exported successfully.");
    }
}

void SettingsDialog::onImportSettings() {
    QString file = QFileDialog::getOpenFileName(this, "Import Settings", QDir::homePath(),
        "JSON Files (*.json)");
    if (file.isEmpty()) return;

    QFile f(file);
    if (!f.open(QIODevice::ReadOnly)) return;

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &error);
    if (error.error != QJsonParseError::NoError) {
        QMessageBox::warning(this, "Error", "Invalid settings file.");
        return;
    }

    QJsonObject obj = doc.object();
    // ... import settings
    loadSettings();
    QMessageBox::information(this, "Success", "Settings imported successfully.");
}

} // namespace gcanner