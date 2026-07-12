#pragma once

#include <QWidget>
#include <QJsonObject>

class QVBoxLayout;
class QHBoxLayout;
class QTabWidget;
class QTreeWidget;
class QTableWidget;
class QLabel;
class QPushButton;
class QComboBox;
class QGroupBox;
class QSplitter;
class QScrollArea;
class QTextEdit;
class QProgressBar;
class QChartView;
class QChart;
class QPieSeries;
class QBarSeries;
class QBarSet;
class QValueAxis;
class QBarCategoryAxis;

namespace gcanner {

class ReportViewer;

class ResultsWidget : public QWidget {
    Q_OBJECT

public:
    explicit ResultsWidget(QWidget* parent = nullptr);

    void loadResults(const QString& filePath);
    void saveResults(const QString& filePath);

signals:
    void exportRequested(const QString& format);

private slots:
    void onExportClicked();
    void onTabChanged(int index);
    void updateCharts();

private:
    void setupUI();
    void setupSummaryTab();
    void setupDetailsTab();
    void setupChartsTab();
    void setupExportTab();
    void populateSummary();
    void populateDetails();
    void createCharts();

    QJsonObject m_results;
    QString m_currentFile;

    QTabWidget* m_tabWidget = nullptr;

    // Summary tab
    QWidget* m_summaryTab = nullptr;
    QLabel* m_gameNameLabel = nullptr;
    QLabel* m_scanDateLabel = nullptr;
    QLabel* m_totalFilesLabel = nullptr;
    QLabel* m_totalSizeLabel = nullptr;
    QLabel* m_scanDurationLabel = nullptr;

    // Requirements labels
    QLabel* m_minCpuLabel = nullptr;
    QLabel* m_minGpuLabel = nullptr;
    QLabel* m_minRamLabel = nullptr;
    QLabel* m_minStorageLabel = nullptr;
    QLabel* m_recCpuLabel = nullptr;
    QLabel* m_recGpuLabel = nullptr;
    QLabel* m_recRamLabel = nullptr;
    QLabel* m_recStorageLabel = nullptr;

    // Details tab
    QWidget* m_detailsTab = nullptr;
    QTreeWidget* m_assetTree = nullptr;
    QTableWidget* m_fileTable = nullptr;

    // Charts tab
    QWidget* m_chartsTab = nullptr;
    QChartView* m_pieChartView = nullptr;
    QChartView* m_barChartView = nullptr;
    QChartView* m_reqChartView = nullptr;

    // Export tab
    QWidget* m_exportTab = nullptr;
    QPushButton* m_exportJsonBtn = nullptr;
    QPushButton* m_exportCsvBtn = nullptr;
    QPushButton* m_exportHtmlBtn = nullptr;
    QPushButton* m_exportMdBtn = nullptr;
};

} // namespace gcanner