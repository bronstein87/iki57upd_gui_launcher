#ifndef UPDATERHELPER_H
#define UPDATERHELPER_H
#include <QObject>
#include <QTimer>
#include <ftps.h>
#include <QFile>
#include <QProcess>




class UpdaterHelper : public QObject
{
    Q_OBJECT
public:
    UpdaterHelper() : ftps(new FTPS)
    {
        ftps->setConnectionSettings("", "", "");
    }
    ~UpdaterHelper() {}
    void handleWrongArgs()
    {
        emit timeToQuit();
    }
    void updateExe(const QStringList& local, const QStringList& remote, const QVector <qint32>& toExecute)
    {
        if (local.size() != remote.size())
        {
            emit errorMessage("Неверно составлен файл обновлений");
            return;
        }
        try
        {
            auto lambda = [this, local, remote, toExecute]()
            {
                static int counter = 0;

                if (toExecute.contains(counter))
                {
                    QProcess* p = new QProcess();
                    connect(p, &QProcess::errorOccurred, this, [this, p]()
                    {
                        emit this->errorMessage(p->errorString());
                    });
                    QString path = local.last();
                    p->start(path.append("\"").prepend("\""));
                }

                if (counter < local.size() - 1)
                {
                    ++counter;
                }
                else
                {
                    counter = 0;
                    emit errorMessage("Успешно завершено!");
                    emit timeToQuit();
                }


            };
            connect (ftps.data(), FTPS::loadFinished, lambda);
            QDir dir;
            const QString dumpPath = "Предыдущая версия";
            dir.mkdir("Предыдущая версия");
            QString filename;
            QString tmpLocal;
            for (int i = 0; i < local.size(); i++)
            {
                if (local[i].contains("/"))
                {
                    int pos = local[i].indexOf(QRegExp("(/)(?!.+/)"), 0);
                    filename = local[i].mid(pos);
                    tmpLocal = local[i];
                    tmpLocal.remove(i, tmpLocal.size() - i);
                    tmpLocal = tmpLocal + "/" + dumpPath + "/" + filename;
                }
                else
                {
                    tmpLocal = dumpPath + "/" + local[i];
                }
                QFile file(local[i]);
                QDateTime dt = ftps->getFileTime(remote[i]);
                QFileInfo info(file);
                if (dt.isValid() && info.lastModified() < dt)
                {
                    file.copy(tmpLocal);
                    file.setPermissions(QFile::ReadOther | QFile::WriteOther);
                    file.remove();
                    ftps->getFile(remote[i], local[i]);
                }
                else
                {
                   emit errorMessage(QString("Файл %1 не нуждается в обновлении").arg(local[i]));
                }
            }
            emit errorMessage("Все файлы проверены на предмет наличия обновлений.");
        }
        catch(std::exception& e)
        {
            emit errorMessage(QString(e.what()));
        }
    }
signals:
    void timeToQuit();
    void errorMessage(const QString& str);
private:
    QScopedPointer <FTPS> ftps;

};

#endif // UPDATERHELPER_H
