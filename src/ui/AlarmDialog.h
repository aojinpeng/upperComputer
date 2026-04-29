#ifndef ALARMDIALOG_H
#define ALARMDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include "../common/Defines.h"

QT_BEGIN_NAMESPACE
namespace Ui { class AlarmDialog; }
QT_END_NAMESPACE

class AlarmDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AlarmDialog(QWidget *parent = nullptr);
    ~AlarmDialog();

public slots:
    void refreshAlarms();

private slots:
    void onConfirmClicked();
    void onConfirmAllClicked();
    void onCloseClicked();

private:
    void initUi();

private:
    Ui::AlarmDialog *ui;
    QList<QPair<qint64, DataPointPtr>> m_currentAlarms;
};

#endif // ALARMDIALOG_H