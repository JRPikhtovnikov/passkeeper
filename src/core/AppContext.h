#pragma once

#include <QByteArray>

struct AppContext {
    QByteArray encryptionKey;
    int clipboardClearSeconds = 30;
};

