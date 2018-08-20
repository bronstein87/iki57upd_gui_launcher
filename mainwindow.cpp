#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QSettings>


MainWindow::MainWindow(const QStringList &args, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow), arguments(args)
{
    ui->setupUi(this);
    ui->progressBar->setRange(0, 100);
    ui->progressBar->setValue(0);
    QTimer::singleShot(1000, this, &MainWindow::startUpdate);
}

void MainWindow::startUpdate()
{
    UpdaterHelper helper;
    QObject::connect(&helper, &UpdaterHelper::errorMessage, [this](auto& str) {ui->infoLabel->setText(str);});
    QObject::connect(&helper, &UpdaterHelper::timeToQuit, [this](){QTimer::singleShot(5000, qApp->quit);});

    QFile file("config.ini");
    if (!file.exists())
    {
        ui->infoLabel->setText("Не найден файл кофигурации config.ini");
        helper.handleWrongArgs();
    }
    else
    {
            ui->progressBar->setValue(20);
            QStringList remote;
            QStringList local;
            QVector <qint32> toExecute;
            QSettings settings("config.ini", QSettings::IniFormat);
            settings.setIniCodec( "UTF-8" );
            QStringList list;
            settings.beginGroup("Local_Files");
            list = settings.allKeys();
            for (int i = 0; i < list.size(); i++)
            {
                local.append(settings.value(list[i]).toString());
                if (local.last().contains(">>exe"))
                {
                    toExecute.append(i);
                    local.last() = local.last().remove(">>exe");
                }

            }
            settings.endGroup();
            settings.beginGroup("Remote_Files");
            list = settings.allKeys();
            for (int i = 0; i < list.size(); i++)
            {
                remote.append(settings.value(list[i]).toString());
            }
            settings.endGroup();

            ui->progressBar->setValue(50);

            if (local.size() != remote.size())
            {
                ui->infoLabel->setText("Неверно составлен файл конфигурации.");
                helper.handleWrongArgs();
            }
            else
            {
                ui->progressBar->setValue(70);
                helper.updateExe(local, remote, toExecute);
            }
    }

    ui->progressBar->setValue(100);
}


MainWindow::~MainWindow()
{
    delete ui;
}
