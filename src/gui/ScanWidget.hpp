#pragma once

#include <QWidget>
#include <QDir>

class QVBoxLayout;
class QHBoxLayout;
class QLabel;
class QPushButton;
class QLineEdit;
class QCheckBox;
class QSpinBox;
class QComboBox;
class QTextEdit;
class QProgressBar;
class QGroupBox;
class QTreeWidget;
class QListWidget;
class QTabWidget;
class QSplitter;
class QScrollArea;
class QFileDialog;
class QMessageBox;
class QThread;
class QProcess;

namespace gcanner {

class ScanWorker;

class ScanWidget : public QWidget {
    Q_OBJECT

public:
    explicit ScanWidget(QWidget* parent = nullptr);

signals:
    void scanStarted();
    void scanProgress(int progress, const QString& status);
    void scanFinished(const QString& resultPath);
    void scanError(const QString& error);

public slots:
    void startNewScan();
    void stopScan();

private slots:
    void onBrowseClicked();
    void onScanTypeChanged(int index);
    void onWorkerProgress(int progress, const QString& status);
    void onWorkerFinished(const QString& resultPath);
    void onWorkerError(const QString& error);
    void updateElapsedTime();

private:
    void setupUI();
    void setupScanOptions();
    void setupOutputOptions();
    void setupLogArea();
    void resetUI();
    QString getSelectedPath() const;

    // UI Components
    QWidget* m_scanConfigWidget = nullptr;
    QWidget* m_scanProgressWidget = nullptr;
    QStackedWidget* m_stackedWidget = nullptr;

    // Config page
    QLineEdit* m_pathEdit = nullptr;
    QPushButton* m_browseBtn = nullptr;
    QCheckBox* m_recursiveCheck = nullptr;
    QCheckBox* m_followSymlinksCheck = nullptr;
    QSpinBox* m_maxDepthSpin = nullptr;
    QSpinBox* m_threadCountSpin = nullptr;
    QCheckBox* m_verifyChecksumsCheck = nullptr;
    QComboBox* m_scanTypeCombo = nullptr;

    // Analyzers group
    QGroupBox* m_analyzersGroup = nullptr;
    QCheckBox* m_texturesCheck = nullptr;
    QCheckBox* m_modelsCheck = nullptr;
    QCheckBox* m_audioCheck = nullptr;
    QCheckBox* m_scriptsCheck = nullptr;
    QCheckBox* m_executablesCheck = nullptr;
    QCheckBox* m_shadersCheck = nullptr;
    QCheckBox* m_particlesCheck = nullptr;
    QCheckBox* m_physicsCheck = nullptr;
    QCheckBox* m_aiCheck = nullptr;
    QCheckBox* m_networkCheck = nullptr;

    // Output options
    QGroupBox* m_outputGroup = nullptr;
    QComboBox* m_outputFormatCombo = nullptr;
    QLineEdit* m_outputPathEdit = nullptr;
    QPushButton* m_outputBrowseBtn = nullptr;
    QCheckBox* m_openAfterScanCheck = nullptr;

    // Progress page
    QProgressBar* m_mainProgress = nullptr;
    QProgressBar* m_fileProgress = nullptr;
    QLabel* m_currentFileLabel = nullptr;
    QLabel* m_filesProcessedLabel = nullptr;
    QLabel* m_elapsedTimeLabel = nullptr;
    QLabel* m_etaLabel = nullptr;
    QTextEdit* m_logOutput = nullptr;
    QPushButton* m_stopBtn = nullptr;
    QPushButton* m_viewResultsBtn = nullptr;

    // Worker
    ScanWorker* m_worker = nullptr;
    QThread* m_workerThread = nullptr;
    QTimer* m_timer = nullptr;
    QTime m_startTime;
    int m_totalFiles = 0;
    int m_processedFiles = 0;
};

} // namespace gcanner