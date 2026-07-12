#include "HardwareBrowserWidget.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QMessageBox>
#include <QFileDialog>
#include <QChartView>
#include <QChart>
#include <QBarSeries>
#include <QBarSet>
#include <QValueAxis>
#include <QBarCategoryAxis>
#include <QTimer>

namespace gcanner {

HardwareBrowserWidget::HardwareBrowserWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadHardwareData();
}

void HardwareBrowserWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Toolbar
    QWidget* toolbar = new QWidget();
    toolbar->setFixedHeight(56);
    toolbar->setStyleSheet("background: #181825; border-bottom: 1px solid #313244;");
    QHBoxLayout* toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(16, 8, 16, 8);
    toolbarLayout->setSpacing(12);

    QLabel* titleLabel = new QLabel("Hardware Database");
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #cdd6f4;");
    toolbarLayout->addWidget(titleLabel);
    toolbarLayout->addStretch();

    m_categoryCombo = new QComboBox();
    m_categoryCombo->addItems({"All", "CPUs", "GPUs", "RAM", "Storage"});
    m_categoryCombo->setFixedWidth(140);
    connect(m_categoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &HardwareBrowserWidget::onCategoryChanged);
    toolbarLayout->addWidget(m_categoryCombo);

    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("Search hardware...");
    m_searchEdit->setFixedWidth(250);
    m_searchEdit->setClearButtonEnabled(true);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &HardwareBrowserWidget::onSearchTextChanged);
    toolbarLayout->addWidget(m_searchEdit);

    m_refreshBtn = new QPushButton("Refresh");
    m_refreshBtn->setCursor(Qt::PointingHandCursor);
    connect(m_refreshBtn, &QPushButton::clicked, this, &HardwareBrowserWidget::onRefreshClicked);
    toolbarLayout->addWidget(m_refreshBtn);

    m_exportBtn = new QPushButton("Export");
    m_exportBtn->setCursor(Qt::PointingHandCursor);
    connect(m_exportBtn, &QPushButton::clicked, this, &HardwareBrowserWidget::onExportClicked);
    toolbarLayout->addWidget(m_exportBtn);

    mainLayout->addWidget(toolbar);

    // Tab widget
    m_tabWidget = new QTabWidget();
    m_tabWidget->setDocumentMode(true);

    // Create tables
    m_cpuTable = createTable({"Name", "Vendor", "Architecture", "Cores", "Threads", "Base Clock", "Boost Clock", "TDP", "Socket", "Release", "PassMark", "Price"});
    m_gpuTable = createTable({"Name", "Vendor", "Architecture", "VRAM", "VRAM Type", "Base Clock", "Boost Clock", "TDP", "Release", "3DMark", "Price", "Perf/$"});
    m_ramTable = createTable({"Name", "Capacity", "Type", "Speed", "CAS Latency", "Voltage", "ECC", "Price", "Perf/$"});
    m_storageTable = createTable({"Name", "Capacity", "Interface", "Form Factor", "Seq Read", "Seq Write", "Rand Read", "Rand Write", "Endurance", "Price", "Price/GB"});

    m_tabWidget->addTab(m_cpuTable, "CPUs");
    m_tabWidget->addTab(m_gpuTable, "GPUs");
    m_tabWidget->addTab(m_ramTable, "RAM");
    m_tabWidget->addTab(m_storageTable, "Storage");

    mainLayout->addWidget(m_tabWidget, 1);
}

QTableWidget* HardwareBrowserWidget::createTable(const QStringList& headers) {
    QTableWidget* table = new QTableWidget();
    table->setColumnCount(headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->setAlternatingRowColors(true);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setSortingEnabled(true);
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table->verticalHeader()->setVisible(false);
    table->setShowGrid(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    return table;
}

void HardwareBrowserWidget::loadHardwareData() {
    // Try to load from embedded resource or cache
    QFile file(":/data/hardware_database.json");
    if (!file.open(QIODevice::ReadOnly)) {
        // Generate sample data
        generateSampleData();
    } else {
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
        if (error.error == QJsonParseError::NoError) {
            QJsonObject root = doc.object();
            m_cpuData = root["cpus"].toArray();
            m_gpuData = root["gpus"].toArray();
            m_ramData = root["ram"].toArray();
            m_storageData = root["storage"].toArray();
        } else {
            generateSampleData();
        }
    }

    populateCPUTab();
    populateGPUTab();
    populateRAMTab();
    populateStorageTab();
}

void HardwareBrowserWidget::generateSampleData() {
    // CPUs
    m_cpuData = QJsonArray{
        QJsonObject{{"name", "Intel Core i9-13900K"}, {"vendor", "Intel"}, {"arch", "Raptor Lake"}, {"cores", 24}, {"threads", 32}, {"base_clock", 3.0}, {"boost_clock", 5.8}, {"tdp", 125}, {"socket", "LGA1700"}, {"release", "2022-Q4"}, {"passmark", 46500}, {"price", 589}},
        QJsonObject{{"name", "AMD Ryzen 9 7950X"}, {"vendor", "AMD"}, {"arch", "Zen 4"}, {"cores", 16}, {"threads", 32}, {"base_clock", 4.5}, {"boost_clock", 5.7}, {"tdp", 170}, {"socket", "AM5"}, {"release", "2022-Q3"}, {"passmark", 45200}, {"price", 699}},
        QJsonObject{{"name", "Intel Core i7-13700K"}, {"vendor", "Intel"}, {"arch", "Raptor Lake"}, {"cores", 16}, {"threads", 24}, {"base_clock", 3.4}, {"boost_clock", 5.4}, {"tdp", 125}, {"socket", "LGA1700"}, {"release", "2022-Q4"}, {"passmark", 38900}, {"price", 409}},
        QJsonObject{{"name", "AMD Ryzen 7 7800X3D"}, {"vendor", "AMD"}, {"arch", "Zen 4"}, {"cores", 8}, {"threads", 16}, {"base_clock", 4.2}, {"boost_clock", 5.0}, {"tdp", 120}, {"socket", "AM5"}, {"release", "2023-Q1"}, {"passmark", 34500}, {"price", 449}},
        QJsonObject{{"name", "Intel Core i5-13600K"}, {"vendor", "Intel"}, {"arch", "Raptor Lake"}, {"cores", 14}, {"threads", 20}, {"base_clock", 3.5}, {"boost_clock", 5.1}, {"tdp", 125}, {"socket", "LGA1700"}, {"release", "2022-Q4"}, {"passmark", 31200}, {"price", 319}},
        QJsonObject{{"name", "AMD Ryzen 5 7600X"}, {"vendor", "AMD"}, {"arch", "Zen 4"}, {"cores", 6}, {"threads", 12}, {"base_clock", 4.7}, {"boost_clock", 5.3}, {"tdp", 105}, {"socket", "AM5"}, {"release", "2022-Q3"}, {"passmark", 23400}, {"price", 299}},
    };

    // GPUs
    m_gpuData = QJsonArray{
        QJsonObject{{"name", "NVIDIA RTX 4090"}, {"vendor", "NVIDIA"}, {"arch", "Ada Lovelace"}, {"vram", 24}, {"vram_type", "GDDR6X"}, {"base_clock", 2235}, {"boost_clock", 2520}, {"tdp", 450}, {"release", "2022-Q4"}, {"3dmark", 36500}, {"price", 1599}, {"perf_dollar", 22.8}},
        QJsonObject{{"name", "NVIDIA RTX 4080"}, {"vendor", "NVIDIA"}, {"arch", "Ada Lovelace"}, {"vram", 16}, {"vram_type", "GDDR6X"}, {"base_clock", 2205}, {"boost_clock", 2505}, {"tdp", 320}, {"release", "2022-Q4"}, {"3dmark", 29800}, {"price", 1199}, {"perf_dollar", 24.9}},
        QJsonObject{{"name", "AMD RX 7900 XTX"}, {"vendor", "AMD"}, {"arch", "RDNA 3"}, {"vram", 24}, {"vram_type", "GDDR6"}, {"base_clock", 1855}, {"boost_clock", 2499}, {"tdp", 355}, {"release", "2022-Q4"}, {"3dmark", 28500}, {"price", 999}, {"perf_dollar", 28.5}},
        QJsonObject{{"name", "NVIDIA RTX 4070 Ti"}, {"vendor", "NVIDIA"}, {"arch", "Ada Lovelace"}, {"vram", 12}, {"vram_type", "GDDR6X"}, {"base_clock", 2310}, {"boost_clock", 2610}, {"tdp", 285}, {"release", "2023-Q1"}, {"3dmark", 24800}, {"price", 799}, {"perf_dollar", 31.0}},
        QJsonObject{{"name", "AMD RX 7900 XT"}, {"vendor", "AMD"}, {"arch", "RDNA 3"}, {"vram", 20}, {"vram_type", "GDDR6"}, {"base_clock", 1500}, {"boost_clock", 2394}, {"tdp", 315}, {"release", "2022-Q4"}, {"3dmark", 24200}, {"price", 899}, {"perf_dollar", 26.9}},
        QJsonObject{{"name", "NVIDIA RTX 3080"}, {"vendor", "NVIDIA"}, {"arch", "Ampere"}, {"vram", 10}, {"vram_type", "GDDR6X"}, {"base_clock", 1440}, {"boost_clock", 1710}, {"tdp", 320}, {"release", "2020-Q3"}, {"3dmark", 21500}, {"price", 699}, {"perf_dollar", 30.8}},
    };

    // RAM
    m_ramData = QJsonArray{
        QJsonObject{{"name", "Corsair Dominator Platinum RGB 64GB (2x32GB)"}, {"capacity", 64}, {"type", "DDR5"}, {"speed", 6000}, {"cas", 36}, {"voltage", 1.35}, {"ecc", false}, {"price", 399}, {"perf_dollar", 0.16}},
        QJsonObject{{"name", "G.Skill Trident Z5 RGB 64GB (2x32GB)"}, {"capacity", 64}, {"type", "DDR5"}, {"speed", 6400}, {"cas", 32}, {"voltage", 1.4}, {"ecc", false}, {"price", 349}, {"perf_dollar", 0.18}},
        QJsonObject{{"name", "Kingston Fury Beast 32GB (2x16GB)"}, {"capacity", 32}, {"type", "DDR5"}, {"speed", 5600}, {"cas", 40}, {"voltage", 1.25}, {"ecc", false}, {"price", 149}, {"perf_dollar", 0.21}},
        QJsonObject{{"name", "Crucial 32GB (2x16GB) DDR5"}, {"capacity", 32}, {"type", "DDR5"}, {"speed", 4800}, {"cas", 40}, {"voltage", 1.1}, {"ecc", false}, {"price", 119}, {"perf_dollar", 0.25}},
        QJsonObject{{"name", "Samsung 32GB DDR5-4800 ECC"}, {"capacity", 32}, {"type", "DDR5"}, {"speed", 4800}, {"cas", 40}, {"voltage", 1.1}, {"ecc", true}, {"price", 189}, {"perf_dollar", 0.16}},
    };

    // Storage
    m_storageData = QJsonArray{
        QJsonObject{{"name", "Samsung 990 Pro 2TB"}, {"capacity", 2000}, {"interface", "PCIe 4.0 x4"}, {"form", "M.2 2280"}, {"seq_read", 7450}, {"seq_write", 6900}, {"rand_read", 1400000}, {"rand_write", 1550000}, {"endurance", 1200}, {"price", 219}, {"price_gb", 0.11}},
        QJsonObject{{"name", "WD Black SN850X 2TB"}, {"capacity", 2000}, {"interface", "PCIe 4.0 x4"}, {"form", "M.2 2280"}, {"seq_read", 7300}, {"seq_write", 6600}, {"rand_read", 1200000}, {"rand_write", 1300000}, {"endurance", 1200}, {"price", 199}, {"price_gb", 0.10}},
        QJsonObject{{"name", "Seagate FireCuda 530 2TB"}, {"capacity", 2000}, {"interface", "PCIe 4.0 x4"}, {"form", "M.2 2280"}, {"seq_read", 7300}, {"seq_write", 6900}, {"rand_read", 1000000}, {"rand_write", 1000000}, {"endurance", 2550}, {"price", 229}, {"price_gb", 0.11}},
        QJsonObject{{"name", "Samsung 870 EVO 2TB"}, {"capacity", 2000}, {"interface", "SATA 6Gb/s"}, {"form", "2.5\""}, {"seq_read", 560}, {"seq_write", 530}, {"rand_read", 98000}, {"rand_write", 88000}, {"endurance", 1200}, {"price", 169}, {"price_gb", 0.08}},
        QJsonObject{{"name", "WD Blue SN570 1TB"}, {"capacity", 1000}, {"interface", "PCIe 3.0 x4"}, {"form", "M.2 2280"}, {"seq_read", 3500}, {"seq_write", 3000}, {"rand_read", 410000}, {"rand_write", 400000}, {"endurance", 600}, {"price", 79}, {"price_gb", 0.08}},
    };
}

void HardwareBrowserWidget::populateTable(QTableWidget* table, const QJsonArray& data) {
    table->setRowCount(data.size());
    for (int i = 0; i < data.size(); ++i) {
        QJsonObject obj = data[i].toObject();
        QStringList keys = obj.keys();
        for (int j = 0; j < qMin(keys.size(), table->columnCount()); ++j) {
            QTableWidgetItem* item = new QTableWidgetItem(obj[keys[j]].toString());
            item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            table->setItem(i, j, item);
        }
    }
    table->resizeColumnsToContents();
    if (table->columnCount() > 0) {
        table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    }
}

void HardwareBrowserWidget::populateCPUTab() {
    populateTable(m_cpuTable, m_cpuData);
}

void HardwareBrowserWidget::populateGPUTab() {
    populateTable(m_gpuTable, m_gpuData);
}

void HardwareBrowserWidget::populateRAMTab() {
    populateTable(m_ramTable, m_ramData);
}

void HardwareBrowserWidget::populateStorageTab() {
    populateTable(m_storageTable, m_storageData);
}

void HardwareBrowserWidget::onCategoryChanged(int index) {
    // Filter tabs based on category
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        bool show = (index == 0);
        if (index == 1) show = (i == 0); // CPUs
        else if (index == 2) show = (i == 1); // GPUs
        else if (index == 3) show = (i == 2); // RAM
        else if (index == 4) show = (i == 3); // Storage
        m_tabWidget->setTabVisible(i, show);
    }
}

void HardwareBrowserWidget::onSearchTextChanged(const QString& text) {
    QTableWidget* currentTable = qobject_cast<QTableWidget*>(m_tabWidget->currentWidget());
    if (!currentTable) return;

    for (int row = 0; row < currentTable->rowCount(); ++row) {
        bool match = text.isEmpty();
        if (!match) {
            for (int col = 0; col < currentTable->columnCount(); ++col) {
                QTableWidgetItem* item = currentTable->item(row, col);
                if (item && item->text().contains(text, Qt::CaseInsensitive)) {
                    match = true;
                    break;
                }
            }
        }
        currentTable->setRowHidden(row, !match);
    }
}

void HardwareBrowserWidget::onRefreshClicked() {
    loadHardwareData();
    QMessageBox::information(this, "Refreshed", "Hardware database refreshed.");
}

void HardwareBrowserWidget::onExportClicked() {
    QString file = QFileDialog::getSaveFileName(this, "Export Hardware Data", QDir::homePath() + "/hardware_export.json",
        "JSON Files (*.json);;CSV Files (*.csv)");
    if (file.isEmpty()) return;

    QJsonObject root;
    root["cpus"] = m_cpuData;
    root["gpus"] = m_gpuData;
    root["ram"] = m_ramData;
    root["storage"] = m_storageData;

    QJsonDocument doc(root);
    QFile f(file);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(doc.toJson(QJsonDocument::Indented));
        QMessageBox::information(this, "Success", "Hardware data exported successfully.");
    }
}

} // namespace gcanner