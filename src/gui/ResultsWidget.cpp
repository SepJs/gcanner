#include "ResultsWidget.hpp"
#include "ReportViewer.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDateTime>
#include <QDir>
#include <QHeaderView>
#include <QScrollArea>
#include <QChartView>
#include <QChart>
#include <QPieSeries>
#include <QBarSeries>
#include <QBarSet>
#include <QValueAxis>
#include <QBarCategoryAxis>
#include <QPieSlice>
#include <QMessageBox>

namespace gcanner {

ResultsWidget::ResultsWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void ResultsWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Header
    QWidget* header = new QWidget();
    header->setFixedHeight(60);
    header->setStyleSheet("background: #181825; border-bottom: 1px solid #313244;");
    QHBoxLayout* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(24, 0, 24, 0);

    QLabel* titleLabel = new QLabel("Scan Results");
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #cdd6f4;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();

    m_exportJsonBtn = new QPushButton("Export JSON");
    m_exportJsonBtn->setCursor(Qt::PointingHandCursor);
    m_exportJsonBtn->setMinimumHeight(36);
    connect(m_exportJsonBtn, &QPushButton::clicked, [this](){ emit exportRequested("json"); });

    m_exportCsvBtn = new QPushButton("Export CSV");
    m_exportCsvBtn->setCursor(Qt::PointingHandCursor);
    m_exportCsvBtn->setMinimumHeight(36);
    connect(m_exportCsvBtn, &QPushButton::clicked, [this](){ emit exportRequested("csv"); });

    m_exportHtmlBtn = new QPushButton("Export HTML");
    m_exportHtmlBtn->setCursor(Qt::PointingHandCursor);
    m_exportHtmlBtn->setMinimumHeight(36);
    connect(m_exportHtmlBtn, &QPushButton::clicked, [this](){ emit exportRequested("html"); });

    m_exportMdBtn = new QPushButton("Export Markdown");
    m_exportMdBtn->setCursor(Qt::PointingHandCursor);
    m_exportMdBtn->setMinimumHeight(36);
    connect(m_exportMdBtn, &QPushButton::clicked, [this](){ emit exportRequested("markdown"); });

    headerLayout->addWidget(m_exportJsonBtn);
    headerLayout->addWidget(m_exportCsvBtn);
    headerLayout->addWidget(m_exportHtmlBtn);
    headerLayout->addWidget(m_exportMdBtn);

    mainLayout->addWidget(header);

    // Tab widget
    m_tabWidget = new QTabWidget();
    m_tabWidget->setDocumentMode(true);
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &ResultsWidget::onTabChanged);
    mainLayout->addWidget(m_tabWidget, 1);

    setupSummaryTab();
    setupDetailsTab();
    setupChartsTab();
    setupExportTab();
}

void ResultsWidget::setupSummaryTab() {
    m_summaryTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_summaryTab);
    layout->setSpacing(16);
    layout->setContentsMargins(24, 24, 24, 24);

    // Game info card
    QGroupBox* infoGroup = new QGroupBox("Game Information");
    QFormLayout* infoLayout = new QFormLayout(infoGroup);
    infoLayout->setSpacing(12);
    infoLayout->setLabelAlignment(Qt::AlignLeft);

    m_gameNameLabel = new QLabel("-");
    m_gameNameLabel->setStyleSheet("font-weight: bold; color: #cdd6f4;");
    infoLayout->addRow("Game:", m_gameNameLabel);

    m_scanDateLabel = new QLabel("-");
    infoLayout->addRow("Scan Date:", m_scanDateLabel);

    m_totalFilesLabel = new QLabel("-");
    infoLayout->addRow("Total Files:", m_totalFilesLabel);

    m_totalSizeLabel = new QLabel("-");
    infoLayout->addRow("Total Size:", m_totalSizeLabel);

    m_scanDurationLabel = new QLabel("-");
    infoLayout->addRow("Scan Duration:", m_scanDurationLabel);

    layout->addWidget(infoGroup);

    // Requirements cards
    QWidget* reqWidget = new QWidget();
    QHBoxLayout* reqLayout = new QHBoxLayout(reqWidget);
    reqLayout->setSpacing(16);

    auto createReqCard = [](const QString& title, const QString& color) {
        QFrame* card = new QFrame();
        card->setFrameShape(QFrame::StyledPanel);
        card->setStyleSheet(QString("background: %1; border-radius: 12px;").arg(color));
        QVBoxLayout* cl = new QVBoxLayout(card);
        cl->setSpacing(8);

        QLabel* tLabel = new QLabel(title);
        tLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: white;");
        tLabel->setAlignment(Qt::AlignCenter);
        cl->addWidget(tLabel);

        return card;
    };

    // Minimum requirements
    QFrame* minCard = new QFrame();
    minCard->setFrameShape(QFrame::StyledPanel);
    minCard->setStyleSheet("background: #313244; border: 1px solid #45475a; border-radius: 12px;");
    QVBoxLayout* minLayout = new QVBoxLayout(minCard);
    minLayout->setSpacing(10);

    QLabel* minTitle = new QLabel("Minimum Requirements");
    minTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #a6e3a1;");
    minTitle->setAlignment(Qt::AlignCenter);
    minLayout->addWidget(minTitle);

    m_minCpuLabel = new QLabel("CPU: -");
    m_minCpuLabel->setStyleSheet("color: #cdd6f4; font-size: 13px;");
    minLayout->addWidget(m_minCpuLabel);

    m_minGpuLabel = new QLabel("GPU: -");
    m_minGpuLabel->setStyleSheet("color: #cdd6f4; font-size: 13px;");
    minLayout->addWidget(m_minGpuLabel);

    m_minRamLabel = new QLabel("RAM: -");
    m_minRamLabel->setStyleSheet("color: #cdd6f4; font-size: 13px;");
    minLayout->addWidget(m_minRamLabel);

    m_minStorageLabel = new QLabel("Storage: -");
    m_minStorageLabel->setStyleSheet("color: #cdd6f4; font-size: 13px;");
    minLayout->addWidget(m_minStorageLabel);

    reqLayout->addWidget(minCard);

    // Recommended requirements
    QFrame* recCard = new QFrame();
    recCard->setFrameShape(QFrame::StyledPanel);
    recCard->setStyleSheet("background: #313244; border: 1px solid #45475a; border-radius: 12px;");
    QVBoxLayout* recLayout = new QVBoxLayout(recCard);
    recLayout->setSpacing(10);

    QLabel* recTitle = new QLabel("Recommended Requirements");
    recTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #89b4fa;");
    recTitle->setAlignment(Qt::AlignCenter);
    recLayout->addWidget(recTitle);

    m_recCpuLabel = new QLabel("CPU: -");
    m_recCpuLabel->setStyleSheet("color: #cdd6f4; font-size: 13px;");
    recLayout->addWidget(m_recCpuLabel);

    m_recGpuLabel = new QLabel("GPU: -");
    m_recGpuLabel->setStyleSheet("color: #cdd6f4; font-size: 13px;");
    recLayout->addWidget(m_recGpuLabel);

    m_recRamLabel = new QLabel("RAM: -");
    m_recRamLabel->setStyleSheet("color: #cdd6f4; font-size: 13px;");
    recLayout->addWidget(m_recRamLabel);

    m_recStorageLabel = new QLabel("Storage: -");
    m_recStorageLabel->setStyleSheet("color: #cdd6f4; font-size: 13px;");
    recLayout->addWidget(m_recStorageLabel);

    reqLayout->addWidget(recCard);

    layout->addWidget(reqWidget);

    // High/Ultra cards
    QWidget* extraWidget = new QWidget();
    QHBoxLayout* extraLayout = new QHBoxLayout(extraWidget);
    extraLayout->setSpacing(16);

    QFrame* highCard = new QFrame();
    highCard->setFrameShape(QFrame::StyledPanel);
    highCard->setStyleSheet("background: #313244; border: 1px solid #45475a; border-radius: 12px;");
    QVBoxLayout* highLayout = new QVBoxLayout(highCard);
    QLabel* highTitle = new QLabel("High Requirements");
    highTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #f9e2af;");
    highTitle->setAlignment(Qt::AlignCenter);
    highLayout->addWidget(highTitle);
    QLabel* highDesc = new QLabel("1440p High / 4K Medium\nCPU: High-end / GPU: RTX 3070+ / RAM: 32GB");
    highDesc->setStyleSheet("color: #a6adc8; font-size: 12px;");
    highDesc->setAlignment(Qt::AlignCenter);
    highLayout->addWidget(highDesc);
    extraLayout->addWidget(highCard);

    QFrame* ultraCard = new QFrame();
    ultraCard->setFrameShape(QFrame::StyledPanel);
    ultraCard->setStyleSheet("background: #313244; border: 1px solid #45475a; border-radius: 12px;");
    QVBoxLayout* ultraLayout = new QVBoxLayout(ultraCard);
    QLabel* ultraTitle = new QLabel("Ultra Requirements");
    ultraTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #f38ba8;");
    ultraTitle->setAlignment(Qt::AlignCenter);
    ultraLayout->addWidget(ultraTitle);
    QLabel* ultraDesc = new QLabel("4K Ultra / Ray Tracing\nCPU: Top-tier / GPU: RTX 4080+ / RAM: 64GB");
    ultraDesc->setStyleSheet("color: #a6adc8; font-size: 12px;");
    ultraDesc->setAlignment(Qt::AlignCenter);
    ultraLayout->addWidget(ultraDesc);
    extraLayout->addWidget(ultraCard);

    layout->addWidget(extraWidget);
    layout->addStretch();

    m_tabWidget->addTab(m_summaryTab, "Summary");
}

void ResultsWidget::setupDetailsTab() {
    m_detailsTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_detailsTab);
    layout->setContentsMargins(0, 0, 0, 0);

    QSplitter* splitter = new QSplitter(Qt::Horizontal);

    // Asset tree
    QWidget* treeWidget = new QWidget();
    QVBoxLayout* treeLayout = new QVBoxLayout(treeWidget);
    treeLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* treeLabel = new QLabel("Asset Breakdown");
    treeLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #cdd6f4; padding: 12px 16px;");
    treeLayout->addWidget(treeLabel);

    m_assetTree = new QTreeWidget();
    m_assetTree->setHeaderLabels({"Asset Type", "Count", "Size", "Details"});
    m_assetTree->setAlternatingRowColors(true);
    m_assetTree->setRootIsDecorated(true);
    m_assetTree->header()->setStretchLastSection(true);
    m_assetTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_assetTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_assetTree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    treeLayout->addWidget(m_assetTree);

    splitter->addWidget(treeWidget);

    // File table
    QWidget* tableWidget = new QWidget();
    QVBoxLayout* tableLayout = new QVBoxLayout(tableWidget);
    tableLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* tableLabel = new QLabel("Top Files by Size");
    tableLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #cdd6f4; padding: 12px 16px;");
    tableLayout->addWidget(tableLabel);

    m_fileTable = new QTableWidget();
    m_fileTable->setColumnCount(4);
    m_fileTable->setHorizontalHeaderLabels({"File", "Type", "Size", "Path"});
    m_fileTable->setAlternatingRowColors(true);
    m_fileTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_fileTable->horizontalHeader()->setStretchLastSection(true);
    m_fileTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_fileTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_fileTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_fileTable->verticalHeader()->setVisible(false);
    tableLayout->addWidget(m_fileTable);

    splitter->addWidget(tableWidget);
    splitter->setSizes({600, 600});

    layout->addWidget(splitter);
    m_tabWidget->addTab(m_detailsTab, "Details");
}

void ResultsWidget::setupChartsTab() {
    m_chartsTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_chartsTab);
    layout->setSpacing(16);
    layout->setContentsMargins(16, 16, 16, 16);

    // Charts in a grid
    QWidget* chartsWidget = new QWidget();
    QGridLayout* chartsLayout = new QGridLayout(chartsWidget);
    chartsLayout->setSpacing(16);

    // Asset distribution pie chart
    QGroupBox* pieGroup = new QGroupBox("Asset Distribution");
    QVBoxLayout* pieLayout = new QVBoxLayout(pieGroup);
    m_pieChartView = new QChartView();
    m_pieChartView->setRenderHint(QPainter::Antialiasing);
    m_pieChartView->setMinimumHeight(300);
    pieLayout->addWidget(m_pieChartView);
    chartsLayout->addWidget(pieGroup, 0, 0);

    // Requirements bar chart
    QGroupBox* barGroup = new QGroupBox("System Requirements Comparison");
    QVBoxLayout* barLayout = new QVBoxLayout(barGroup);
    m_barChartView = new QChartView();
    m_barChartView->setRenderHint(QPainter::Antialiasing);
    m_barChartView->setMinimumHeight(300);
    barLayout->addWidget(m_barChartView);
    chartsLayout->addWidget(barGroup, 0, 1);

    // File types chart
    QGroupBox* fileGroup = new QGroupBox("File Types");
    QVBoxLayout* fileLayout = new QVBoxLayout(fileGroup);
    QChartView* fileChartView = new QChartView();
    fileChartView->setRenderHint(QPainter::Antialiasing);
    fileChartView->setMinimumHeight(300);
    fileLayout->addWidget(fileChartView);
    chartsLayout->addWidget(fileGroup, 1, 0);

    // Estimated performance chart
    QGroupBox* perfGroup = new QGroupBox("Estimated Performance");
    QVBoxLayout* perfLayout = new QVBoxLayout(perfGroup);
    m_reqChartView = new QChartView();
    m_reqChartView->setRenderHint(QPainter::Antialiasing);
    m_reqChartView->setMinimumHeight(300);
    perfLayout->addWidget(m_reqChartView);
    chartsLayout->addWidget(perfGroup, 1, 1);

    layout->addWidget(chartsWidget);
    m_tabWidget->addTab(m_chartsTab, "Charts");
}

void ResultsWidget::setupExportTab() {
    m_exportTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_exportTab);
    layout->setAlignment(Qt::AlignTop);
    layout->setSpacing(16);
    layout->setContentsMargins(24, 24, 24, 24);

    QLabel* title = new QLabel("Export Report");
    title->setStyleSheet("font-size: 24px; font-weight: bold; color: #cdd6f4;");
    layout->addWidget(title);

    QLabel* desc = new QLabel("Export the scan results in various formats for documentation, sharing, or further analysis.");
    desc->setStyleSheet("font-size: 14px; color: #a6adc8;");
    desc->setWordWrap(true);
    layout->addWidget(desc);

    QWidget* btnWidget = new QWidget();
    QGridLayout* btnLayout = new QGridLayout(btnWidget);
    btnLayout->setSpacing(16);

    auto createExportBtn = [this](const QString& text, const QString& desc, const QString& format) {
        QFrame* card = new QFrame();
        card->setFrameShape(QFrame::StyledPanel);
        card->setCursor(Qt::PointingHandCursor);
        card->setStyleSheet("background: #181825; border: 1px solid #313244; border-radius: 12px; padding: 20px;");
        QVBoxLayout* cl = new QVBoxLayout(card);
        cl->setSpacing(8);

        QLabel* tLabel = new QLabel(text);
        tLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #cdd6f4;");
        cl->addWidget(tLabel);

        QLabel* dLabel = new QLabel(desc);
        dLabel->setStyleSheet("color: #a6adc8; font-size: 13px;");
        dLabel->setWordWrap(true);
        cl->addWidget(dLabel);

        QPushButton* btn = new QPushButton("Export");
        btn->setCursor(Qt::PointingHandCursor);
        btn->setMinimumHeight(40);
        connect(btn, &QPushButton::clicked, [this, format](){ emit exportRequested(format); });
        cl->addWidget(btn);

        return card;
    };

    btnLayout->addWidget(createExportBtn("JSON", "Complete structured data for programmatic use", "json"), 0, 0);
    btnLayout->addWidget(createExportBtn("CSV", "Spreadsheet-compatible tabular data", "csv"), 0, 1);
    btnLayout->addWidget(createExportBtn("HTML", "Rich formatted report with charts", "html"), 1, 0);
    btnLayout->addWidget(createExportBtn("Markdown", "Documentation-ready format", "markdown"), 1, 1);

    layout->addWidget(btnWidget);
    layout->addStretch();

    m_tabWidget->addTab(m_exportTab, "Export");
}

void ResultsWidget::loadResults(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Error", "Could not open results file.");
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        QMessageBox::warning(this, "Error", QString("Invalid JSON: %1").arg(error.errorString()));
        return;
    }

    m_results = doc.object();
    m_currentFile = filePath;
    populateSummary();
    populateDetails();
    createCharts();
}

void ResultsWidget::saveResults(const QString& filePath) {
    QJsonDocument doc(m_results);
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
    }
}

void ResultsWidget::populateSummary() {
    if (m_results.isEmpty()) return;

    QJsonObject gameInfo = m_results["game_info"].toObject();
    QJsonObject requirements = m_results["requirements"].toObject();
    QJsonObject scanInfo = m_results["scan_info"].toObject();

    m_gameNameLabel->setText(gameInfo["name"].toString("Unknown"));
    m_scanDateLabel->setText(QDateTime::fromSecsSinceEpoch(scanInfo["timestamp"].toInt()).toString("yyyy-MM-dd HH:mm:ss"));
    m_totalFilesLabel->setText(QString::number(scanInfo["total_files"].toInt()));
    m_totalSizeLabel->setText(formatBytes(scanInfo["total_size"].toInt()));
    m_scanDurationLabel->setText(QString("%1 sec").arg(scanInfo["duration_sec"].toInt()));

    // Minimum requirements
    QJsonObject minReq = requirements["minimum"].toObject();
    m_minCpuLabel->setText(QString("CPU: %1 @ %2 GHz (%3C/%4T)")
        .arg(minReq["cpu_vendor"].toString())
        .arg(minReq["cpu_base_clock"].toDouble(), 0, 'f', 1)
        .arg(minReq["cpu_cores"].toInt())
        .arg(minReq["cpu_threads"].toInt()));
    m_minGpuLabel->setText(QString("GPU: %1 %2 (%3 GB %4 VRAM)")
        .arg(minReq["gpu_vendor"].toString())
        .arg(minReq["gpu_architecture"].toString())
        .arg(minReq["gpu_vram"].toInt() / 1024)
        .arg(minReq["gpu_vram_type"].toString()));
m_minRamLabel->setText(QString("RAM: %1 GB %2 @ %3 MT/s")
        .arg(minReq["ram_capacity"].toInt() / 1024)
        .arg(minReq["ram_type"].toString())
        .arg(minReq["ram_speed"].toInt()));
    m_minStorageLabel->setText(QString("Storage: %1 %2 GB (%3 MB/s read)")
        .arg(minReq["storage_type"].toString())
        .arg(minReq["storage_capacity"].toInt())
        .arg(minReq["storage_read_speed"].toInt()));

    // Recommended requirements
    QJsonObject recReq = requirements["recommended"].toObject();
    m_recCpuLabel->setText(QString("CPU: %1 @ %2 GHz (%3C/%4T)")
        .arg(recReq["cpu_vendor"].toString())
        .arg(recReq["cpu_base_clock"].toDouble(), 0, 'f', 1)
        .arg(recReq["cpu_cores"].toInt())
        .arg(recReq["cpu_threads"].toInt()));
    m_recGpuLabel->setText(QString("GPU: %1 %2 (%3 GB %4 VRAM)")
        .arg(recReq["gpu_vendor"].toString())
        .arg(recReq["gpu_architecture"].toString())
        .arg(recReq["gpu_vram"].toInt() / 1024)
        .arg(recReq["gpu_vram_type"].toString()));
    m_recRamLabel->setText(QString("RAM: %1 GB %2 @ %3 MT/s")
        .arg(recReq["ram_capacity"].toInt() / 1024)
        .arg(recReq["ram_type"].toString())
        .arg(recReq["ram_speed"].toInt()));
    m_recStorageLabel->setText(QString("Storage: %1 %2 GB (%3 MB/s read)")
        .arg(recReq["storage_type"].toString())
        .arg(recReq["storage_capacity"].toInt())
        .arg(recReq["storage_read_speed"].toInt()));
}

void ResultsWidget::populateDetails() {
    if (m_results.isEmpty()) return;

    // Asset tree
    QJsonObject assets = m_results["aggregated_assets"].toObject();
    m_assetTree->clear();

    QStringList categories = {"textures", "models", "audio", "scripts", "executables", "shaders", "particles", "physics", "ai", "network"};
    QStringList displayNames = {"🖼️ Textures", "📦 Models", "🔊 Audio", "📜 Scripts", "⚙️ Executables", "🎨 Shaders", "✨ Particles", "⚡ Physics", "🧠 AI", "🌐 Network"};

    for (int i = 0; i < categories.size(); ++i) {
        QJsonObject cat = assets[categories[i]].toObject();
        if (cat.isEmpty()) continue;

        QTreeWidgetItem* item = new QTreeWidgetItem(m_assetTree);
        item->setText(0, displayNames[i]);
        item->setText(1, QString::number(cat["count"].toInt()));
        item->setText(2, formatBytes(cat["total_size"].toInt()));
        item->setText(3, cat["summary"].toString());

        // Add sub-items
        if (cat.contains("formats")) {
            QJsonObject formats = cat["formats"].toObject();
            for (auto it = formats.begin(); it != formats.end(); ++it) {
                QTreeWidgetItem* sub = new QTreeWidgetItem(item);
                sub->setText(0, it.key());
                sub->setText(1, QString::number(it.value().toInt()));
            }
        }
    }
    m_assetTree->expandAll();

    // File table - top 20 files by size
    QJsonArray files = m_results["top_files"].toArray();
    m_fileTable->setRowCount(qMin(files.size(), 20));
    for (int i = 0; i < qMin(files.size(), 20); ++i) {
        QJsonObject f = files[i].toObject();
        m_fileTable->setItem(i, 0, new QTableWidgetItem(f["name"].toString()));
        m_fileTable->setItem(i, 1, new QTableWidgetItem(f["type"].toString()));
        m_fileTable->setItem(i, 2, new QTableWidgetItem(formatBytes(f["size"].toInt())));
        m_fileTable->setItem(i, 3, new QTableWidgetItem(f["path"].toString()));
    }
}

void ResultsWidget::createCharts() {
    if (m_results.isEmpty()) return;

    QJsonObject assets = m_results["aggregated_assets"].toObject();

    // Pie chart - Asset distribution
    QPieSeries* pieSeries = new QPieSeries();
    QStringList categories = {"textures", "models", "audio", "scripts", "executables", "shaders", "particles", "physics", "ai", "network"};
    QStringList displayNames = {"Textures", "Models", "Audio", "Scripts", "Executables", "Shaders", "Particles", "Physics", "AI", "Network"};

    for (int i = 0; i < categories.size(); ++i) {
        QJsonObject cat = assets[categories[i]].toObject();
        int count = cat["count"].toInt();
        if (count > 0) {
            QPieSlice* slice = pieSeries->append(displayNames[i], count);
            slice->setLabelVisible(true);
            slice->setLabel(QString("%1 (%2%)").arg(displayNames[i]).arg(count));
        }
    }

    QChart* pieChart = new QChart();
    pieChart->addSeries(pieSeries);
    pieChart->setTitle("Asset Distribution by Count");
    pieChart->setAnimationOptions(QChart::SeriesAnimations);
    pieChart->legend()->setVisible(true);
    pieChart->legend()->setAlignment(Qt::AlignRight);
    m_pieChartView->setChart(pieChart);

    // Bar chart - Requirements comparison
    QBarSet* minSet = new QBarSet("Minimum");
    QBarSet* recSet = new QBarSet("Recommended");

    QJsonObject minReq = m_results["requirements"].toObject()["minimum"].toObject();
    QJsonObject recReq = m_results["requirements"].toObject()["recommended"].toObject();

    *minSet << (minReq["cpu_base_clock"].toDouble() / 5.0 * 100)
            << (minReq["gpu_vram"].toInt() / 24576.0 * 100)
            << (minReq["ram_capacity"].toInt() / 65536.0 * 100)
            << (minReq["storage_read_speed"].toInt() / 7000.0 * 100);

    *recSet << (recReq["cpu_base_clock"].toDouble() / 5.0 * 100)
            << (recReq["gpu_vram"].toInt() / 24576.0 * 100)
            << (recReq["ram_capacity"].toInt() / 65536.0 * 100)
            << (recReq["storage_read_speed"].toInt() / 7000.0 * 100);

    QBarSeries* barSeries = new QBarSeries();
    barSeries->append(minSet);
    barSeries->append(recSet);

    QChart* barChart = new QChart();
    barChart->addSeries(barSeries);
    barChart->setTitle("Requirements Comparison (Normalized %)");
    barChart->setAnimationOptions(QChart::SeriesAnimations);

    QStringList categories2 = {"CPU", "GPU VRAM", "RAM", "Storage"};
    QBarCategoryAxis* axisX = new QBarCategoryAxis();
    axisX->append(categories2);
    barChart->addAxis(axisX, Qt::AlignBottom);
    barSeries->attachAxis(axisX);

    QValueAxis* axisY = new QValueAxis();
    axisY->setRange(0, 100);
    axisY->setLabelFormat("%d%");
    barChart->addAxis(axisY, Qt::AlignLeft);
    barSeries->attachAxis(axisY);

    barChart->legend()->setVisible(true);
    barChart->legend()->setAlignment(Qt::AlignBottom);
    m_barChartView->setChart(barChart);
}

QString ResultsWidget::formatBytes(qint64 bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int i = 0;
    double val = bytes;
    while (val >= 1024 && i < 4) {
        val /= 1024;
        i++;
    }
    return QString("%1 %2").arg(val, 0, 'f', 2).arg(units[i]);
}

void ResultsWidget::onTabChanged(int index) {
    if (index == 2) { // Charts tab
        createCharts();
    }
}

void ResultsWidget::onExportClicked() {
    // Handled by button connections
}

} // namespace gcanner