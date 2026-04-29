#ifndef DATAQUERYDIALOG_H
#define DATAQUERYDIALOG_H

#include <QDialog>
#include <QDateTimeEdit>
#include <QComboBox>
#include "../common/Defines.h"
#include "ui/ChartWidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class DataQueryDialog; }
QT_END_NAMESPACE

class DataQueryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DataQueryDialog(QWidget *parent = nullptr);
    ~DataQueryDialog();

private slots:
    void onQueryClicked();
    void onCloseClicked();

private:
    void initUi();
    void loadDevices();

private:
    Ui::DataQueryDialog *ui;
    ChartWidget* m_chartWidget;
};

#endif // DATAQUERYDIALOG_H