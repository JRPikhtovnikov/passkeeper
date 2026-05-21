#include "services/ClipboardService.h"

#include <QClipboard>
#include <QGuiApplication>

namespace services {

ClipboardService::ClipboardService(QObject* parent)
    : QObject(parent)
{
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &ClipboardService::clearIfUnchanged);
}

void ClipboardService::copySecret(const QString& value, int clearAfterSeconds)
{
    m_lastCopied = value;
    QGuiApplication::clipboard()->setText(value, QClipboard::Clipboard);
    m_timer.start(clearAfterSeconds * 1000);
}

void ClipboardService::clearIfUnchanged()
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    if (clipboard->text(QClipboard::Clipboard) == m_lastCopied) {
        clipboard->clear(QClipboard::Clipboard);
    }
    m_lastCopied.clear();
}

} // namespace services
