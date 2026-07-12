#include "ScanWorker.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QThread>

ScanWorker::ScanWorker(QObject* parent)
    : QObject(parent)
{
}

void ScanWorker::setPath(const QString& path) { m_path = path; }
void ScanWorker::setAnalyzers(const QStringList& analyzers) { m_analyzers = analyzers; }
void ScanWorker::setRecursive(bool recursive) { m_recursive = recursive; }
void ScanWorker::setFollowSymlinks(bool follow) { m_followSymlinks = follow; }
void ScanWorker::setMaxDepth(int depth) { m_maxDepth = depth; }
void ScanWorker::setThreadCount(int threads) { m_threadCount = threads; }
void ScanWorker::setVerifyChecksums(bool verify) { m_verifyChecksums = verify; }
void ScanWorker::setOutputFormat(const QString& format) { m_outputFormat = format; }
void ScanWorker::setOutputDir(const QString& dir) { m_outputDir = dir; }
void ScanWorker::requestStop() { m_stopRequested = true; }

void ScanWorker::process() {
    if (m_path.isEmpty()) {
        emit error("No path specified");
        return;
    }

    QDir dir(m_path);
    if (!dir.exists()) {
        emit error(QString("Directory does not exist: %1").arg(m_path));
        return;
    }

    if (m_analyzers.isEmpty()) {
        emit error("No analyzers selected");
        return;
    }

    // Simulate scanning
    QTime timer;
    timer.start();

    // File discovery
    emit progress(5, "Discovering files...");
    QThread::msleep(300);
    if (m_stopRequested) { emit error("Scan cancelled"); return; }

    // Process each analyzer
    int totalAnalyzers = m_analyzers.size();
    for (int i = 0; i < totalAnalyzers; ++i) {
        if (m_stopRequested) break;

        int baseProgress = 10 + (i * 80 / totalAnalyzers);
        emit progress(baseProgress, QString("Analyzing %1...").arg(m_analyzers[i]));

        for (int j = 0; j < 5; ++j) {
            QThread::msleep(150);
            if (m_stopRequested) break;
            int progress = baseProgress + (j * 15 / 5);
            emit progress(progress, QString("Analyzing %1... (%2/5)").arg(m_analyzers[i]).arg(j + 1));
        }
    }

    if (m_stopRequested) {
        emit error("Scan cancelled by user");
        return;
    }

    // Aggregation
    emit progress(95, "Aggregating results...");
    QThread::msleep(500);

    emit progress(100, "Generating report...");

    // Generate mock result
    QString result = generateMockResult();

    // Save to file
    QString outputDir = m_outputDir.isEmpty() ? m_path : m_outputDir;
    QString fileName = QString("gcanner_report_%1.%2")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"))
        .arg(m_outputFormat);
    m_resultPath = QDir(outputDir).filePath(fileName);

    QFile file(m_resultPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(result.toUtf8());
        file.close();
    }

    emit finished(m_resultPath);
}

QString ScanWorker::generateMockResult() {
    QJsonObject root;
    root["scan_info"] = QJsonObject{
        {"tool", "gcanner"}, {"version", "1.0.0"},
        {"scan_date", QDateTime::currentDateTime().toString(Qt::ISODate)},
        {"scan_path", m_path},
        {"analyzers", QJsonArray::fromStringList(m_analyzers)},
        {"scan_duration_sec", 5}
    };

    QJsonObject assets;
    if (m_analyzers.contains("textures")) {
        assets["textures"] = QJsonObject{{"count", 1247}, {"total_size", 2147483648}, {"summary", "DDS: 512, PNG: 389, JPEG: 346"},
            {"formats", QJsonObject{{"DDS", 512}, {"PNG", 389}, {"JPEG", 346}}}};
    }
    if (m_analyzers.contains("models")) {
        assets["models"] = QJsonObject{{"count", 342}, {"total_size", 5368709120}, {"summary", "FBX: 123, GLTF: 98, OBJ: 121"},
            {"formats", QJsonObject{{"FBX", 123}, {"GLTF", 98}, {"OBJ", 121}}}};
    }
    if (m_analyzers.contains("audio")) {
        assets["audio"] = QJsonObject{{"count", 892}, {"total_size", 1073741824}, {"summary", "OGG: 445, WAV: 234, MP3: 213"},
            {"formats", QJsonObject{{"OGG", 445}, {"WAV", 234}, {"MP3", 213}}}};
    }
    if (m_analyzers.contains("scripts")) {
        assets["scripts"] = QJsonObject{{"count", 1567}, {"total_size", 52428800}, {"summary", "C#: 623, Lua: 445, Python: 289, JS: 210"},
            {"formats", QJsonObject{{"C#", 623}, {"Lua", 445}, {"Python", 289}, {"JavaScript", 210}}}};
    }
    if (m_analyzers.contains("executables")) {
        assets["executables"] = QJsonObject{{"count", 23}, {"total_size", 268435456}, {"summary", "PE: 15, ELF: 8"},
            {"formats", QJsonObject{{"PE", 15}, {"ELF", 8}}}};
    }
    if (m_analyzers.contains("shaders")) {
        assets["shaders"] = QJsonObject{{"count", 456}, {"total_size", 104857600}, {"summary", "HLSL: 189, GLSL: 134, SPIR-V: 87, MSL: 46"},
            {"formats", QJsonObject{{"HLSL", 189}, {"GLSL", 134}, {"SPIR-V", 87}, {"MSL", 46}}}};
    }
    if (m_analyzers.contains("particles")) {
        assets["particles"] = QJsonObject{{"count", 89}, {"total_size", 20971520}, {"summary", "PFX: 45, PTX: 32, PAR: 12"},
            {"formats", QJsonObject{{"PFX", 45}, {"PTX", 32}, {"PAR", 12}}}};
    }
    if (m_analyzers.contains("physics")) {
        assets["physics"] = QJsonObject{{"count", 34}, {"total_size", 52428800}, {"summary", "PhysX: 18, Havok: 9, Bullet: 7"},
            {"formats", QJsonObject{{"PhysX", 18}, {"Havok", 9}, {"Bullet", 7}}}};
    }
    if (m_analyzers.contains("ai")) {
        assets["ai"] = QJsonObject{{"count", 156}, {"total_size", 31457280}, {"summary", "NavMesh: 45, BehaviorTree: 38, StateMachine: 42, NeuralNet: 31"},
            {"formats", QJsonObject{{"NavMesh", 45}, {"BehaviorTree", 38}, {"StateMachine", 42}, {"NeuralNet", 31}}}};
    }
    if (m_analyzers.contains("network")) {
        assets["network"] = QJsonObject{{"count", 23}, {"total_size", 10485760}, {"summary", "Photon: 8, Mirror: 6, Steam: 5, EOS: 4"},
            {"formats", QJsonObject{{"Photon", 8}, {"Mirror", 6}, {"Steam", 5}, {"EOS", 4}}}};
    }

    root["aggregated_assets"] = assets;

    // Requirements
    QJsonObject req;
    req["minimum"] = QJsonObject{
        {"os_minimum", "Windows 10 64-bit"}, {"os_recommended", "Windows 10/11 64-bit"},
        {"cpu_vendor", "Intel"}, {"cpu_arch", "Core i5-8400"}, {"cpu_base_clock", 2.8}, {"cpu_cores", 6}, {"cpu_threads", 6},
        {"gpu_vendor", "NVIDIA"}, {"gpu_architecture", "GTX 1060"}, {"gpu_vram", 6144}, {"gpu_vram_type", "GDDR5"},
        {"ram_capacity", 16384}, {"ram_type", "DDR4"}, {"ram_speed", 2666},
        {"storage_type", "SSD"}, {"storage_capacity", 50}, {"storage_read_speed", 500},
        {"dx_version", "11"}, {"vk_version", "1.1"}, {"metal_version", "2.0"}
    };
    req["recommended"] = QJsonObject{
        {"os_minimum", "Windows 10 64-bit"}, {"os_recommended", "Windows 11 64-bit"},
        {"cpu_vendor", "AMD"}, {"cpu_arch", "Ryzen 7 5800X"}, {"cpu_base_clock", 3.8}, {"cpu_cores", 8}, {"cpu_threads", 16},
        {"gpu_vendor", "NVIDIA"}, {"gpu_architecture", "RTX 3070"}, {"gpu_vram", 8192}, {"gpu_vram_type", "GDDR6"},
        {"ram_capacity", 32768}, {"ram_type", "DDR4"}, {"ram_speed", 3600},
        {"storage_type", "NVMe SSD"}, {"storage_capacity", 100}, {"storage_read_speed", 3500},
        {"dx_version", "12"}, {"vk_version", "1.3"}, {"metal_version", "3.0"}
    };
    root["requirements"] = req;

    QJsonDocument doc(root);
    return doc.toJson(QJsonDocument::Indented);
}