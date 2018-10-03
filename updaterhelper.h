#ifndef UPDATERHELPER_H
#define UPDATERHELPER_H
#include <QObject>
#include <QTimer>
#include <ftps.h>
#include <QFile>
#include <QProcess>
#include <windows.h>


class UpdaterHelper : public QObject
{
    Q_OBJECT
public:
    UpdaterHelper() : ftps(new FTPS)
    {
        ftps->setConnectionSettings("ftpuser", "/j=@WJ205", "ftp://193.232.17.58:21");
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
            bool isSomethingUpdated = false;
            auto lambda = [this, local, remote, toExecute](auto str)
            {
                static int counter = 1;

                for (int i = 0; i < remote.size(); i++)
                {
                    QString tmp = remote[i];
                    if (tmp.contains("\\"))
                    {
                        tmp.replace("\\", "/");
                    }
                    if (tmp == str)
                    {
                        if (toExecute.contains(i))
                        {
                            this->startProcess(local[i]);
                        }
                        break;
                    }
                }


                if (counter < local.size())
                {
                    ++counter;
                }
                else
                {
                    counter = 0;
                    emit this->errorMessage("Успешно завершено!");
                    emit this->timeToQuit();
                }


            };
            connect (ftps.data(), &FTPS::loadFinished, lambda);
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
                QString remoteFile = remote[i];
                remoteFile.replace("\\", "/");
                QDateTime dt = ftps->getFileTime(remoteFile);
                QFileInfo info(file);
                if (dt.isValid() && info.lastModified() < dt)
                {
                    file.copy(tmpLocal);
                    file.setPermissions(QFile::ReadOther | QFile::WriteOther);
                    file.remove();
                    ftps->getFile(remoteFile, local[i]);
                    isSomethingUpdated = true;
                }
                else
                {
                    if (toExecute.contains(i))
                    {
                        startProcess(local[i]);
                    }
                    emit errorMessage(QString("Файл %1 не нуждается в обновлении").arg(local[i]));
                }
            }
            emit errorMessage("Все файлы проверены на предмет наличия обновлений.");
            if (!isSomethingUpdated)
            {
                emit timeToQuit();
            }
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

    void startProcess (const QString& file)
    {
        QProcess* p = new QProcess();
        connect(p, &QProcess::errorOccurred, this, [this, p]()
        {
            emit this->errorMessage(p->errorString());
        });
        QString path = file;
        p->start(path.append("\"").prepend("\""));
    }

    QString getVersionString(const QString& fName)
    {
        // first of all, GetFileVersionInfoSize
        DWORD dwHandle;
        QString f = "C:/Users/Yumatov/Documents/GitHub/build-iki57upd_gui_launcher-Desktop_Qt_5_9_4_MinGW_32bit4-Debug/BOKZDatabase.exe";
        DWORD dwLen = GetFileVersionInfoSize(f.toStdWString().c_str(), &dwHandle);
        // GetFileVersionInfo
        LPSTR lpData = new char[dwLen];
        if(!GetFileVersionInfo(fName.toStdWString().c_str(), dwHandle, dwLen, lpData))
        {
            qDebug() << "error in GetFileVersionInfo";
            qDebug() << GetLastError();
            delete[] lpData;
            return QString();
        }

        // VerQueryValue
        VS_FIXEDFILEINFO *lpBuffer = nullptr;
        UINT uLen;

        if(!VerQueryValue(lpData,
                          QString("\\").toStdWString().c_str(),
                          (LPVOID*)&lpBuffer,
                          &uLen))
        {
            qDebug() << "error in VerQueryValue";
            delete[] lpData;
            return QString();
        }
        return  QString::number( (lpBuffer->dwFileVersionMS >> 16 ) & 0xffff ) + "." +
                QString::number( ( lpBuffer->dwFileVersionMS) & 0xffff ) + "." +
                QString::number( ( lpBuffer->dwFileVersionLS >> 16 ) & 0xffff ) + "." +
                QString::number( ( lpBuffer->dwFileVersionLS) & 0xffff );

    }

    QScopedPointer <FTPS> ftps;

};

#endif // UPDATERHELPER_H
