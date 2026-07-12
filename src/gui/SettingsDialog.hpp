#pragma once

#include <QDialog>
#include <QSettings>

class QVBoxLayout;
class QTabWidget;
class QWidget;
class QGroupBox;
class QCheckBox;
class QSpinBox;
class QComboBox;
class QLineEdit;
class QPushButton;
class QDialogButtonBox;
class QFormLayout;
class QLabel;

namespace gcanner {

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);
    ~SettingsDialog() override;

private slots:
    void onAccepted();
    void onRejected();
    void onBrowseCacheDir();
    void onBrowseDataDir();
    void onResetDefaults();
    void onExportSettings();
    void onImportSettings();

private:
    void setupUI();
    void setupGeneralTab();
    void setupScanningTab();
    void setupAnalyzersTab();
    void setupOutputTab();
    void setupAdvancedTab();
    void loadSettings();
    void saveSettings();
    void applySettings();

    QSettings m_settings;
    QTabWidget* m_tabWidget = nullptr;
    QDialogButtonBox* m_buttonBox = nullptr;

    // General
    QCheckBox* m_darkThemeCheck = nullptr;
    QCheckBox* m_startMinimizedCheck = nullptr;
    QCheckBox* m_checkUpdatesCheck = nullptr;
    QComboBox* m_languageCombo = nullptr;
    QComboBox* m_startupViewCombo = nullptr;

    // Scanning
    QCheckBox* m_recursiveDefaultCheck = nullptr;
    QSpinBox* m_maxDepthDefaultSpin = nullptr;
    QSpinBox* m_threadCountDefaultSpin = nullptr;
    QCheckBox* m_verifyChecksumsDefaultCheck = nullptr;
    QCheckBox* m_useMmapCheck = nullptr;
    QLineEdit* m_cacheDirEdit = nullptr;
    QLineEdit* m_dataDirEdit = nullptr;

    // Analyzers
    QCheckBox* m_texturesEnabledCheck = nullptr;
    QCheckBox* m_modelsEnabledCheck = nullptr;
    QCheckBox* m_audioEnabledCheck = nullptr;
    QCheckBox* m_scriptsEnabledCheck = nullptr;
    QCheckBox* m_executablesEnabledCheck = nullptr;
    QCheckBox* m_shadersEnabledCheck = nullptr;
    QCheckBox* m_particlesEnabledCheck = nullptr;
    QCheckBox* m_physicsEnabledCheck = nullptr;
    QCheckBox* m_aiEnabledCheck = nullptr;
    QCheckBox* m_networkEnabledCheck = nullptr;

    // Output
    QComboBox* m_defaultFormatCombo = nullptr;
    QCheckBox* m_openAfterScanCheck = nullptr;
    QCheckBox* m_showProgressCheck = nullptr;
    QCheckBox* m_showDetailedLogCheck = nullptr;
    QSpinBox* m_maxDisplayItemsSpin = nullptr;

    // Advanced
    QCheckBox* m_enableTelemetryCheck = nullptr;
    QCheckBox* m_enableCrashReportingCheck = nullptr;
    QSpinBox* m_logLevelSpin = nullptr;
    QLineEdit* m_customArgsEdit = nullptr;
    QPushButton* m_resetBtn = nullptr;
    QPushButton* m_exportBtn = nullptr;
    QPushButton* m_importBtn = nullptr;
};

} // namespace gcanner