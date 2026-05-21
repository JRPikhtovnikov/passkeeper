cmake_minimum_required(VERSION 3.20)

project(PassKeeper VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 QUIET COMPONENTS Quick QuickControls2)
if (Qt6_FOUND)
    set(QT_PACKAGE Qt6)
else()
    find_package(Qt5 REQUIRED COMPONENTS Quick QuickControls2)
    set(QT_PACKAGE Qt5)
endif()

find_package(OpenSSL REQUIRED)
find_package(PostgreSQL REQUIRED)

add_executable(passkeeper
    qml/qml.qrc
    src/main.cpp
    src/core/AppContext.h
    src/crypto/CryptoProvider.cpp
    src/crypto/CryptoProvider.h
    src/crypto/PasswordGenerator.cpp
    src/crypto/PasswordGenerator.h
    src/database/DatabaseManager.cpp
    src/database/DatabaseManager.h
    src/models/Category.h
    src/models/CryptoTypes.h
    src/models/PasswordEntry.h
    src/services/AuthService.cpp
    src/services/AuthService.h
    src/services/CategoryService.cpp
    src/services/CategoryService.h
    src/services/ClipboardService.cpp
    src/services/ClipboardService.h
    src/services/EntryService.cpp
    src/services/EntryService.h
    src/services/ExportImportService.cpp
    src/services/ExportImportService.h
    src/ui/QmlAppController.cpp
    src/ui/QmlAppController.h
    src/utils/Env.cpp
    src/utils/Env.h
    src/utils/DotEnv.cpp
    src/utils/DotEnv.h
)

target_include_directories(passkeeper PRIVATE src)
target_link_libraries(passkeeper PRIVATE ${QT_PACKAGE}::Quick ${QT_PACKAGE}::QuickControls2 OpenSSL::Crypto PostgreSQL::PostgreSQL)

if (APPLE)
    set_target_properties(passkeeper PROPERTIES MACOSX_BUNDLE FALSE)
endif()
