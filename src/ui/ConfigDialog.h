#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QDialog>
#include <QJsonArray>

namespace Ui { class ConfigDialog; }

class ConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConfigDialog(QWidget* parent = nullptr);
    ~ConfigDialog();

private slots:
    void onSaveClicked();
    void onCancelClicked();
    void onAddDeviceClicked();
    void onEditDeviceClicked();
    void onDeleteDeviceClicked();
    void onDeviceSelectionChanged();

private:
    void initUi();
    void loadConfigToUi();
    void refreshDeviceTable();

    Ui::ConfigDialog* ui;
    QJsonArray m_editableDeviceArray;
};

#endif // CONFIGDIALOG_H