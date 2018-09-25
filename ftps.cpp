#include "ftps.h"


static size_t readFcn(void *ptr, size_t size, size_t nmemb, void *stream)
{
    static QDataStream out;
    out.setDevice((QIODevice*) (stream));
    int i = out.readRawData((char*)ptr, size * nmemb);
    return i;
}


static size_t writeFcn (void *buffer, size_t size, size_t nmemb, void *stream)
{
    static QDataStream out;
    out.setDevice((QIODevice*)stream);
    return out.writeRawData((char*)buffer, size * nmemb);
}

FTPS::FTPS()
{
    read_callback = readFcn;
    write_callback = writeFcn;
}

void FTPS::setConnectionSettings(const QString& username, const QString& password, const QString& url)
{

    settings.userpass = username + ":" + password;
    settings.url = url;
}

void FTPS::sendFiles(QStringList& filesFrom, const QStringList& dirTo, bool createDirNoExists)
{
    QStringList files;
    QStringList fullFromFiles;
    for (auto& i : filesFrom)
    {
        QFileInfo info(i);
        int pos = i.indexOf(QRegExp("(/)(?!.+/)"), 0);
        QString removePathPart = i.mid(0, pos);
        if (info.isDir())
        {
            QDirIterator it(i, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                QString fname = it.next();
                if (fname.contains(QRegExp("\\/\\.+")))
                {
                    continue;
                }
                fullFromFiles.append(fname);
                files.append(fname.remove(0, removePathPart.size()));
            }
        }
        else
        {
            QString tmpPath = i;
            fullFromFiles.append(tmpPath);
            files.append(tmpPath.remove(0, pos));
        }
    }
    const qint32 handleCount = files.size();
    CURL* (*handles) = new CURL* [handleCount];

    for (int i = 0; i < handleCount; i++)
    {
        handles[i] = curl_easy_init();
    }

    for (int i = 0; i < handleCount; i++)
    {
        QFile* file = new QFile(fullFromFiles[i]);
        QString fileTo = dirTo[i] + files[i];
        curl_off_t fSize = file->size();
        if (!file->open(QIODevice::ReadOnly))
        {
            throw std::runtime_error("Не удалось открыть файл " + fullFromFiles[i].toStdString());
        }
        QString filePath = QString(settings.url + "/" + fileTo);
        prepareSendHandles(filePath, handles[i], createDirNoExists, file, fSize);
        devices.append((QIODevice*)file);
    }
    CURLM *multiHandle = curl_multi_init();

    for(int i = 0; i < handleCount; i++)
    {
        curl_multi_add_handle(multiHandle, handles[i]);
    }

    int stillRunning = 0;
    curl_multi_perform(multiHandle, &stillRunning);
    timers.append(new QTimer(this));
    QTimer* timer = timers.back();
    auto check =
            [stillRunning, multiHandle, handles, handleCount, timer, this]() mutable
    {
        if (stillRunning)
        {
            handleSendStillRunning(timer, stillRunning, multiHandle);
        }
        else
        {
            handleSendDone(timer, multiHandle, handleCount, handles);
            for (auto i : devices)
            {
                delete i;
            }
            devices.clear();
        }
    };
    timer->setInterval(10);
    QObject::connect(timer, &QTimer::timeout, check);
    timer->start();
}

void FTPS::sendFiles(const QVector<QByteArray *> &data, const QStringList &names, const QStringList &dirTo, bool createDirNoExists)
{
    const qint32 handleCount = data.size();
    CURL* (*handles) = new CURL* [handleCount];

    for(int i = 0; i < handleCount; i++)
    {
        handles[i] = curl_easy_init();
    }

    for (int i = 0; i < handleCount; i++)
    {
        QString filePath = settings.url + "/" + dirTo[i] + "/" + names[i];
        prepareSendHandles(filePath, handles[i], createDirNoExists, data[i], data[i]->size());
    }
    CURLM *multiHandle = curl_multi_init();
    for(int i = 0; i < handleCount; i++)
    {
        curl_multi_add_handle(multiHandle, handles[i]);
    }

    int stillRunning = 0;
    curl_multi_perform(multiHandle, &stillRunning);
    timers.append(new QTimer(this));
    QTimer* timer = timers.back();
    auto check =
            [stillRunning, multiHandle, handles, handleCount, timer, this]() mutable
    {
        if (stillRunning)
        {
            handleSendStillRunning(timer, stillRunning, multiHandle);
        }
        else
        {
            handleSendDone(timer, multiHandle, handleCount, handles);
        }
    };
    timer->setInterval(10);
    QObject::connect(timer, &QTimer::timeout, check);
    timer->start();
}

void FTPS::prepareSendHandles(QString& filePath, CURL* handle, bool createDirNoExists, const void* device, curl_off_t fSize)
{
    filePath.replace("я", "яя"); // ЛАЙФХАК
    curl_easy_setopt(handle, CURLOPT_USERPWD, settings.userpass.toStdString().c_str());
    curl_easy_setopt(handle, CURLOPT_READFUNCTION, read_callback);
    curl_easy_setopt(handle, CURLOPT_USE_SSL, CURLUSESSL_ALL);
    curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, FALSE);
    curl_easy_setopt(handle, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(handle, CURLOPT_URL, filePath.toLocal8Bit().constData());
    curl_easy_setopt(handle, CURLOPT_READDATA, device);
    curl_easy_setopt(handle, CURLOPT_INFILESIZE_LARGE, fSize);
    curl_easy_setopt(handle, CURLOPT_VERBOSE, 1L);

    if (createDirNoExists)
    {
        curl_easy_setopt(handle, CURLOPT_FTP_CREATE_MISSING_DIRS,
                         CURLFTP_CREATE_DIR_RETRY);
    }
}

void FTPS::handleGet(const QString& path, CURLcode& res, const void *device, CURL *curl)
{
    curl_easy_setopt(curl, CURLOPT_USERPWD, settings.userpass.toStdString().c_str());

    curl_easy_setopt(curl, CURLOPT_URL,
                     QString(settings.url + "/" + path).toLocal8Bit().constData());

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, device);

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE);

    curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);

    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    res = curl_easy_perform(curl);

    curl_easy_cleanup(curl);

    if(CURLE_OK != res)
    {
        throw std::runtime_error(QString("Error with status " + QString(curl_easy_strerror(res))).toStdString());
    }

}

void FTPS::getFile(const QString& path, const QString& saveTo)
{
    CURL *curl;
    CURLcode res;
    curl_global_init(CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();
    if(curl) {
        QFile file(saveTo);
        if (!file.open(QIODevice::WriteOnly))
        {
            throw std::runtime_error(saveTo.toStdString() + "Wrong filename.");
        }
        handleGet(path, res, &file, curl);
    }

    curl_global_cleanup();
    emit loadFinished(path);
}


void FTPS::getFile(const QString& path, const QByteArray& writeTo)
{
    CURL *curl;
    CURLcode res;
    curl_global_init(CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();
    if(curl) {
        handleGet(path, res, &writeTo, curl);
    }

    curl_global_cleanup();
    emit loadFinished(path);
}


QDateTime FTPS::getFileTime(const QString& path)
{
    CURL *curl;
    CURLcode res;
    curl_global_init(CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_USERPWD, settings.userpass.toStdString().c_str());

        curl_easy_setopt(curl, CURLOPT_URL,
                         QString(settings.url + "/" + path).toLocal8Bit().constData());

        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE);

        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);

        curl_easy_setopt(curl, CURLOPT_FILETIME, 1L);

        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        res = curl_easy_perform(curl);

        int time;
        res = curl_easy_getinfo(curl, CURLINFO_FILETIME, &time);
        if(CURLE_OK != res)
        {
            throw std::runtime_error(QString("Error with status " + QString(curl_easy_strerror(res))).toStdString());
        }
        if (time == -1)
        {
            return QDateTime();
        }

        curl_easy_cleanup(curl);
        return QDateTime::fromTime_t(time);
    }
    else
    {
        curl_easy_cleanup(curl);
        throw std::runtime_error(QString("Error libcurl init").toStdString());
    }
    emit loadFinished(path);

}

void FTPS::handleSendStillRunning(QTimer* timer, int& stillRunning, CURLM* multiHandle)
{
    fd_set fdread;
    fd_set fdwrite;
    fd_set fdexcep;
    int maxfd = -1;

    FD_ZERO(&fdread);
    FD_ZERO(&fdwrite);
    FD_ZERO(&fdexcep);

    CURLMcode mc = curl_multi_fdset(multiHandle, &fdread, &fdwrite, &fdexcep, &maxfd);

    if(mc != CURLM_OK)
    {
        timer->stop();
        throw std::runtime_error
                ("curl_multi_fdset() failed, code " + QString::number(mc).toStdString());

    }

    int rc;
    if(maxfd == -1)
    {
        rc = 0;
    }
    else
    {
        rc = select(maxfd + 1, &fdread, &fdwrite, &fdexcep, 0);
    }

    switch(rc)
    {
    case -1:
        /* select error */
        break;
    case 0: /* timeout */
    default: /* action */
    {
        curl_multi_perform(multiHandle, &stillRunning);
        break;
    }
    }

}

void FTPS::handleSendDone(QTimer* timer, CURLM* multiHandle, const int handleCount, CURL** handles)
{
    CURLMsg *msg;
    int msgsLeft;
    while ((msg = curl_multi_info_read(multiHandle, &msgsLeft)))
    {
        if (msg->msg == CURLMSG_DONE)
        {
            int found = 0;
            for (int idx = 0; idx < handleCount; idx++)
            {
                found = (msg->easy_handle == handles[idx]);
                if (found)
                    break;
            }
            emit uploadFinished("Result: " + QString(curl_easy_strerror(msg->data.result)));
        }
    }
    curl_multi_cleanup(multiHandle);

    for (int i = 0; i < handleCount; i++)
    {
        curl_easy_cleanup(handles[i]);
    }
    timer->stop();
    timers.removeOne(timer);
    delete timer;
    delete [] handles;
}

FTPS::~FTPS()
{
    for (const auto& i : timers)
    {
        delete i;
    }
}
