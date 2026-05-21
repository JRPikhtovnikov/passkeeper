#pragma once

#include <QObject>
#include <QTimer>

namespace services {

class ClipboardService final : public QObject {
    Q_OBJECT

public:
    explicit ClipboardService(QObject* parent = nullptr);
    void copySecret(const QString& value, int clearAfterSeconds);

private slots:
    void clearIfUnchanged();

private:
    QTimer m_timer;
    QString m_lastCopied;
};

} // namespace services

