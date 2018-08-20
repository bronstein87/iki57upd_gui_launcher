#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <updaterhelper.h>
#include <QStringList>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(const QStringList& args, QWidget *parent = 0);
    void startUpdate();
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QStringList arguments;
};

#endif // MAINWINDOW_H
