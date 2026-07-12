#pragma once

#include <QObject>
#include <QStringList>

class ScanWorker : public QObject {
    Q_OBJECT

public:
    explicit ScanWorker(QObject* parent = nullptr);

    void setPath(const QString& path);
    void setAnalyzers(const QStringList& analyzers);
    void setRecursive(bool recursive);
    void setFollowSymlinks(bool follow);
    void setMaxDepth(int depth);
    void setThreadCount(int threads);
    void setVerifyChecksums(bool verify);
    void setOutputFormat(const QString& format);
    void setOutputDir(const QString& dir);
    void requestStop();
    QString resultPath() const { return m_resultPath; }

public slots:
    void process();

signals:
    void progress(int percent, const QString& status);
    void finished(const QString& resultPath);
    void error(const QString& error);

private:
    QString m_path;
    QStringList m_analyzers;
    bool m_recursive = true;
    bool m_followSymlinks = false;
    int m_maxDepth = 20;
    int m_threadCount = 4;
    bool m_verifyChecksums = false;
    QString m_outputFormat = "json";
    QString m_outputDir;
    QString m_resultPath;
    bool m_stopRequested = false;

    QString runAnalysis();
    void simulateScan();
};