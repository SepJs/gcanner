#pragma once

#include <QWidget>
#include <QJsonArray>

class QTabWidget;
class QTableWidget;
class QTreeWidget;
class QComboBox;
class QLineEdit;
class QPushButton;
class QLabel;
class QGroupBox;
class QVBoxLayout;
class QHBoxLayout;
class QChartView;
class QChart;
class QBarSeries;
class QBarSet;
class QValueAxis;
class QBarCategoryAxis;

namespace gcanner {

class HardwareBrowserWidget : public QWidget {
    Q_OBJECT

public:
    explicit HardwareBrowserWidget(QWidget* parent = nullptr);

private slots:
    void onCategoryChanged(int index);
    void onSearchTextChanged(const QString& text);
    void onRefreshClicked();
    void onExportClicked();

private:
    void setupUI();
    void loadHardwareData();
    void populateCPUTab();
    void populateGPUTab();
    void populateRAMTab();
    void populateStorageTab();
    void applyFilter();

    QTabWidget* m_tabWidget = nullptr;
    QComboBox* m_categoryCombo = nullptr;
    QLineEdit* m_searchEdit = nullptr;
    QPushButton* m_refreshBtn = nullptr;
    QPushButton* m_exportBtn = nullptr;

    QTableWidget* m_cpuTable = nullptr;
    QTableWidget* m_gpuTable = nullptr;
    QTableWidget* m_ramTable = nullptr;
    QTableWidget* m_storageTable = nullptr;

    QJsonArray m_cpuData;
    QJsonArray m_gpuData;
    QJsonArray m_ramData;
    QJsonArray m_storageData;
};

} // namespace gcanner