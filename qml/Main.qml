import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 1180
    height: 760
    minimumWidth: 980
    minimumHeight: 640
    visible: true
    title: "PassKeeper"
    color: theme.window

    QtObject {
        id: theme
        readonly property color window: "#f5f7fb"
        readonly property color panel: "#ffffff"
        readonly property color panelSoft: "#eef2f7"
        readonly property color border: "#d9dee8"
        readonly property color text: "#111827"
        readonly property color muted: "#6b7280"
        readonly property color accent: "#2563eb"
        readonly property color accentSoft: "#dbeafe"
        readonly property color danger: "#dc2626"
        readonly property color success: "#059669"
    }

    property var selectedEntry: ({})
    property bool editingEntry: false
    property bool editingCategory: false

    onEditingEntryChanged: {
        if (editingEntry)
            entryPopup.open()
        else
            entryPopup.close()
    }

    onEditingCategoryChanged: {
        if (editingCategory)
            categoryPopup.open()
        else
            categoryPopup.close()
    }

    Component.onCompleted: {
        if (appController.unlocked)
            appController.reload()
    }

    Connections {
        target: appController
        function onToast(message) {
            toastLabel.text = message
            toast.open()
        }
    }

    Popup {
        id: toast
        x: root.width - width - 28
        y: root.height - height - 28
        width: Math.max(220, toastLabel.implicitWidth + 36)
        height: 46
        modal: false
        closePolicy: Popup.NoAutoClose
        background: Rectangle {
            color: "#111827"
            radius: 8
        }
        Timer {
            interval: 2200
            running: toast.opened
            onTriggered: toast.close()
        }
        Label {
            id: toastLabel
            anchors.centerIn: parent
            color: "#ffffff"
            font.pixelSize: 14
        }
    }

    component IconButton: Button {
        id: iconButton
        property color fg: theme.text
        implicitWidth: 38
        implicitHeight: 34
        font.pixelSize: 15
        contentItem: Text {
            text: iconButton.text
            color: iconButton.fg
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: iconButton.font.pixelSize
            font.bold: true
        }
        background: Rectangle {
            color: iconButton.hovered ? theme.panelSoft : "transparent"
            border.color: iconButton.hovered ? theme.border : "transparent"
            radius: 6
        }
    }

    component PrimaryButton: Button {
        id: primaryButton
        implicitHeight: 40
        leftPadding: 18
        rightPadding: 18
        font.pixelSize: 14
        font.bold: true
        contentItem: Text {
            text: primaryButton.text
            color: "#ffffff"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font: primaryButton.font
        }
        background: Rectangle {
            color: primaryButton.down ? "#1d4ed8" : theme.accent
            radius: 7
        }
    }

    component GhostButton: Button {
        id: ghostButton
        implicitHeight: 38
        leftPadding: 14
        rightPadding: 14
        font.pixelSize: 14
        contentItem: Text {
            text: ghostButton.text
            color: ghostButton.enabled ? theme.text : "#9ca3af"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font: ghostButton.font
        }
        background: Rectangle {
            color: ghostButton.hovered ? theme.panelSoft : theme.panel
            border.color: theme.border
            radius: 7
        }
    }

    component Field: TextField {
        id: field
        color: theme.text
        placeholderTextColor: "#9ca3af"
        selectedTextColor: "#ffffff"
        selectionColor: theme.accent
        font.pixelSize: 14
        background: Rectangle {
            color: theme.panel
            border.color: field.activeFocus ? theme.accent : theme.border
            radius: 7
        }
    }

    component Area: TextArea {
        id: area
        color: theme.text
        placeholderTextColor: "#9ca3af"
        selectedTextColor: "#ffffff"
        selectionColor: theme.accent
        wrapMode: TextArea.Wrap
        font.pixelSize: 14
        background: Rectangle {
            color: theme.panel
            border.color: area.activeFocus ? theme.accent : theme.border
            radius: 7
        }
    }

    Rectangle {
        anchors.fill: parent
        color: theme.window

        Loader {
            anchors.fill: parent
            sourceComponent: appController.unlocked ? vaultView : authView
        }
    }

    Component {
        id: authView

        Item {
            Rectangle {
                width: 420
                anchors.centerIn: parent
                color: theme.panel
                radius: 8
                border.color: theme.border
                height: authColumn.implicitHeight + 52

                ColumnLayout {
                    id: authColumn
                    anchors.fill: parent
                    anchors.margins: 26
                    spacing: 16

                    Label {
                        text: appController.firstRun ? "Create master password" : "Unlock vault"
                        color: theme.text
                        font.pixelSize: 26
                        font.bold: true
                        Layout.fillWidth: true
                    }

                    Label {
                        text: appController.firstRun ? "Your vault key is derived locally and never stored." : "Enter your master password to decrypt local entries."
                        color: theme.muted
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    Field {
                        id: masterPassword
                        placeholderText: "Master password"
                        echoMode: TextInput.Password
                        Layout.fillWidth: true
                        onAccepted: authButton.clicked()
                    }

                    Field {
                        id: masterConfirm
                        visible: appController.firstRun
                        placeholderText: "Repeat password"
                        echoMode: TextInput.Password
                        Layout.fillWidth: true
                        onAccepted: authButton.clicked()
                    }

                    Label {
                        visible: appController.errorMessage.length > 0
                        text: appController.errorMessage
                        color: theme.danger
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    PrimaryButton {
                        id: authButton
                        text: appController.firstRun ? "Create vault" : "Unlock"
                        Layout.fillWidth: true
                        onClicked: {
                            if (appController.firstRun)
                                appController.createMasterPassword(masterPassword.text, masterConfirm.text)
                            else
                                appController.unlock(masterPassword.text)
                        }
                    }
                }
            }
        }
    }

    Component {
        id: vaultView

        RowLayout {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 14

            Rectangle {
                Layout.preferredWidth: 250
                Layout.fillHeight: true
                color: theme.panel
                radius: 8
                border.color: theme.border

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            text: "PassKeeper"
                            color: theme.text
                            font.pixelSize: 22
                            font.bold: true
                            Layout.fillWidth: true
                        }
                        IconButton {
                            text: "+"
                            ToolTip.visible: hovered
                            ToolTip.text: "New category"
                            onClicked: {
                                categoryName.text = ""
                                categoryId.value = -1
                                editingCategory = true
                            }
                        }
                    }

                    ListView {
                        id: categoryList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: appController.categories
                        spacing: 4

                        delegate: Rectangle {
                            required property var modelData
                            width: categoryList.width
                            height: 42
                            radius: 7
                            color: appController.categoryFilter === modelData.id ? theme.accentSoft : "transparent"

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 6
                                Label {
                                    text: modelData.name
                                    color: theme.text
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                                IconButton {
                                    visible: modelData.id > 0
                                    text: "..."
                                    onClicked: {
                                        categoryId.value = modelData.id
                                        categoryName.text = modelData.name
                                        editingCategory = true
                                    }
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                anchors.rightMargin: modelData.id > 0 ? 38 : 0
                                onClicked: appController.categoryFilter = modelData.id
                            }
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 14

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Field {
                        id: search
                        placeholderText: "Search title, URL, category"
                        text: appController.searchText
                        Layout.fillWidth: true
                        onTextChanged: appController.searchText = text
                    }

                    GhostButton {
                        text: "Generator"
                        onClicked: generatorPopup.open()
                    }
                    GhostButton {
                        text: "Import"
                        onClicked: importDialog.open()
                    }
                    GhostButton {
                        text: "Export"
                        onClicked: exportDialog.open()
                    }
                    GhostButton {
                        text: "Settings"
                        onClicked: settingsPopup.open()
                    }
                    PrimaryButton {
                        text: "New entry"
                        onClicked: openEntryEditor({})
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: theme.panel
                    radius: 8
                    border.color: theme.border

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 0

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 34
                            Label { text: "Title"; color: theme.muted; Layout.fillWidth: true; font.bold: true }
                            Label { text: "Username"; color: theme.muted; Layout.preferredWidth: 180; font.bold: true }
                            Label { text: "URL"; color: theme.muted; Layout.preferredWidth: 240; font.bold: true }
                            Label { text: "Category"; color: theme.muted; Layout.preferredWidth: 150; font.bold: true }
                            Item { Layout.preferredWidth: 116 }
                        }

                        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: theme.border }

                        ListView {
                            id: entryList
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            model: appController.entries

                            delegate: Rectangle {
                                required property var modelData
                                width: entryList.width
                                height: 54
                                color: rowMouse.containsMouse ? "#f8fafc" : theme.panel

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 10
                                    anchors.rightMargin: 6
                                    Label {
                                        text: modelData.title
                                        color: theme.text
                                        font.bold: true
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                    Label {
                                        text: modelData.username
                                        color: theme.muted
                                        elide: Text.ElideRight
                                        Layout.preferredWidth: 180
                                    }
                                    Label {
                                        text: modelData.url
                                        color: theme.muted
                                        elide: Text.ElideRight
                                        Layout.preferredWidth: 240
                                    }
                                    Label {
                                        text: modelData.categoryName || "No category"
                                        color: theme.muted
                                        elide: Text.ElideRight
                                        Layout.preferredWidth: 150
                                    }
                                    RowLayout {
                                        Layout.preferredWidth: 116
                                        spacing: 4
                                        IconButton { text: "C"; ToolTip.visible: hovered; ToolTip.text: "Copy password"; onClicked: appController.copyPassword(modelData.id) }
                                        IconButton { text: "E"; ToolTip.visible: hovered; ToolTip.text: "Edit"; onClicked: openEntryEditor(modelData) }
                                        IconButton { text: "X"; fg: theme.danger; ToolTip.visible: hovered; ToolTip.text: "Delete"; onClicked: appController.deleteEntry(modelData.id) }
                                    }
                                }
                                MouseArea {
                                    id: rowMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    acceptedButtons: Qt.NoButton
                                }
                            }

                            Label {
                                anchors.centerIn: parent
                                visible: entryList.count === 0
                                text: "No entries"
                                color: theme.muted
                                font.pixelSize: 18
                            }
                        }
                    }
                }
            }
        }
    }

    function openEntryEditor(entry) {
        selectedEntry = entry
        entryId.value = entry.id || -1
        entryTitle.text = entry.title || ""
        entryCategory.currentIndex = 0
        for (var i = 0; i < appController.categories.length; ++i) {
            if (appController.categories[i].id === (entry.categoryId || -1))
                entryCategory.currentIndex = i
        }
        entryUsername.text = entry.username || ""
        entryPassword.text = entry.password || ""
        entryUrl.text = entry.url || ""
        entryNotes.text = entry.notes || ""
        editingEntry = true
    }

    Popup {
        id: entryPopup
        onClosed: root.editingEntry = false
        anchors.centerIn: Overlay.overlay
        width: 560
        height: 620
        modal: true
        focus: true
        background: Rectangle { color: theme.panel; radius: 8; border.color: theme.border }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 12

            property int labelWidth: 84

            Label {
                text: entryId.value > 0 ? "Edit entry" : "New entry"
                color: theme.text
                font.pixelSize: 24
                font.bold: true
                Layout.fillWidth: true
            }

            SpinBox { id: entryId; visible: false; from: -1; to: 999999999; value: -1 }
            Field { id: entryTitle; placeholderText: "Title"; Layout.fillWidth: true }
            ComboBox {
                id: entryCategory
                Layout.fillWidth: true
                model: appController.categories
                textRole: "name"
                valueRole: "id"
                background: Rectangle { color: theme.panel; border.color: theme.border; radius: 7 }
                contentItem: Text { text: entryCategory.displayText; color: theme.text; verticalAlignment: Text.AlignVCenter; leftPadding: 10 }
            }
            Field { id: entryUsername; placeholderText: "Username"; Layout.fillWidth: true }
            RowLayout {
                Layout.fillWidth: true
                Field { id: entryPassword; placeholderText: "Password"; echoMode: TextInput.Password; Layout.fillWidth: true }
                GhostButton { text: "Generate"; onClicked: { generatorTarget.value = 1; generatorPopup.open() } }
            }
            Field { id: entryUrl; placeholderText: "URL"; Layout.fillWidth: true }
            Area { id: entryNotes; placeholderText: "Notes"; Layout.fillWidth: true; Layout.fillHeight: true }

            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                GhostButton { text: "Cancel"; onClicked: entryPopup.close() }
                PrimaryButton {
                    text: "Save"
                    onClicked: {
                        if (appController.saveEntry(entryId.value, entryCategory.currentValue, entryTitle.text, entryUsername.text, entryPassword.text, entryUrl.text, entryNotes.text))
                            entryPopup.close()
                    }
                }
            }
        }
    }

    Popup {
        id: categoryPopup
        onClosed: root.editingCategory = false
        anchors.centerIn: Overlay.overlay
        width: 380
        height: 190
        modal: true
        focus: true
        background: Rectangle { color: theme.panel; radius: 8; border.color: theme.border }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 12

            Label { text: categoryId.value > 0 ? "Edit category" : "New category"; color: theme.text; font.pixelSize: 22; font.bold: true }
            SpinBox { id: categoryId; visible: false; from: -1; to: 999999999; value: -1 }
            Field { id: categoryName; placeholderText: "Name"; Layout.fillWidth: true }
            RowLayout {
                Layout.fillWidth: true
                GhostButton { visible: categoryId.value > 0; text: "Delete"; onClicked: { appController.deleteCategory(categoryId.value); categoryPopup.close() } }
                Item { Layout.fillWidth: true }
                GhostButton { text: "Cancel"; onClicked: categoryPopup.close() }
                PrimaryButton { text: "Save"; onClicked: { if (appController.saveCategory(categoryId.value, categoryName.text)) categoryPopup.close() } }
            }
        }
    }

    Popup {
        id: generatorPopup
        anchors.centerIn: Overlay.overlay
        width: 460
        height: 360
        modal: true
        focus: true
        background: Rectangle { color: theme.panel; radius: 8; border.color: theme.border }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 12

            SpinBox { id: generatorTarget; visible: false; from: 0; to: 1; value: 0 }
            Label { text: "Password generator"; color: theme.text; font.pixelSize: 22; font.bold: true }
            RowLayout {
                Layout.fillWidth: true
                Label { text: "Length"; color: theme.text; Layout.fillWidth: true }
                SpinBox { id: passwordLength; from: 8; to: 256; value: 20 }
            }
            CheckBox { id: optLower; text: "Lowercase"; checked: true }
            CheckBox { id: optUpper; text: "Uppercase"; checked: true }
            CheckBox { id: optDigits; text: "Digits"; checked: true }
            CheckBox { id: optSymbols; text: "Symbols"; checked: true }
            Field { id: generatedPassword; readOnly: true; Layout.fillWidth: true }
            RowLayout {
                Layout.fillWidth: true
                GhostButton {
                    text: "Regenerate"
                    onClicked: generatedPassword.text = appController.generatePassword(passwordLength.value, optLower.checked, optUpper.checked, optDigits.checked, optSymbols.checked)
                }
                Item { Layout.fillWidth: true }
                PrimaryButton {
                    text: "Use"
                    onClicked: {
                        if (!generatedPassword.text)
                            generatedPassword.text = appController.generatePassword(passwordLength.value, optLower.checked, optUpper.checked, optDigits.checked, optSymbols.checked)
                        if (generatorTarget.value === 1)
                            entryPassword.text = generatedPassword.text
                        generatorTarget.value = 0
                        generatorPopup.close()
                    }
                }
            }
        }
        onOpened: generatedPassword.text = appController.generatePassword(passwordLength.value, optLower.checked, optUpper.checked, optDigits.checked, optSymbols.checked)
    }

    Popup {
        id: settingsPopup
        anchors.centerIn: Overlay.overlay
        width: 400
        height: 200
        modal: true
        focus: true
        background: Rectangle { color: theme.panel; radius: 8; border.color: theme.border }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 16
            Label { text: "Settings"; color: theme.text; font.pixelSize: 22; font.bold: true }
            RowLayout {
                Layout.fillWidth: true
                Label { text: "Clipboard clear, seconds"; color: theme.text; Layout.fillWidth: true }
                SpinBox { id: clearSeconds; from: 5; to: 300; value: appController.clipboardSeconds }
            }
            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                GhostButton { text: "Cancel"; onClicked: settingsPopup.close() }
                PrimaryButton { text: "Save"; onClicked: { appController.clipboardSeconds = clearSeconds.value; settingsPopup.close() } }
            }
        }
    }

    FileDialog {
        id: exportDialog
        title: "Export vault"
        fileMode: FileDialog.SaveFile
        nameFilters: ["PassKeeper export (*.pke)"]
        onAccepted: exportPasswordPopup.open()
    }

    FileDialog {
        id: importDialog
        title: "Import vault"
        fileMode: FileDialog.OpenFile
        nameFilters: ["PassKeeper export (*.pke)"]
        onAccepted: importPasswordPopup.open()
    }

    Popup {
        id: exportPasswordPopup
        anchors.centerIn: Overlay.overlay
        width: 380
        height: 180
        modal: true
        focus: true
        background: Rectangle { color: theme.panel; radius: 8; border.color: theme.border }
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 12
            Label { text: "Export password"; color: theme.text; font.pixelSize: 22; font.bold: true }
            Field { id: exportPassword; echoMode: TextInput.Password; placeholderText: "Password"; Layout.fillWidth: true }
            PrimaryButton { text: "Export"; Layout.alignment: Qt.AlignRight; onClicked: { if (appController.exportVault(exportDialog.selectedFile, exportPassword.text)) exportPasswordPopup.close() } }
        }
    }

    Popup {
        id: importPasswordPopup
        anchors.centerIn: Overlay.overlay
        width: 380
        height: 180
        modal: true
        focus: true
        background: Rectangle { color: theme.panel; radius: 8; border.color: theme.border }
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 12
            Label { text: "Import password"; color: theme.text; font.pixelSize: 22; font.bold: true }
            Field { id: importPassword; echoMode: TextInput.Password; placeholderText: "Password"; Layout.fillWidth: true }
            PrimaryButton { text: "Import"; Layout.alignment: Qt.AlignRight; onClicked: { if (appController.importVault(importDialog.selectedFile, importPassword.text)) importPasswordPopup.close() } }
        }
    }

    Popup {
        id: errorPopup
        visible: appController.errorMessage.length > 0 && appController.unlocked
        anchors.centerIn: Overlay.overlay
        width: 460
        height: Math.max(180, errorText.implicitHeight + 110)
        modal: true
        focus: true
        background: Rectangle { color: theme.panel; radius: 8; border.color: theme.border }
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 12
            Label { text: "Error"; color: theme.danger; font.pixelSize: 22; font.bold: true }
            Label { id: errorText; text: appController.errorMessage; color: theme.text; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            PrimaryButton { text: "OK"; Layout.alignment: Qt.AlignRight; onClicked: appController.clearError() }
        }
    }
}
