#ifndef FTPS_H
#define FTPS_H

#include <QObject>
#include <QTimer>
#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <QVector>
#include <QDebug>
#include <QDataStream>
#include <QFile>
#include <QDirIterator>
#include <QFileInfo>
#include <QTextCodec>
#include <QDateTime>


/* somewhat unix-specific */
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>

/* curl stuff */
#include <libcurl/curl.h>

struct FTPSSettings
{
    QString userpass;
    QString url;
};




class FTPS : public QObject
{
    Q_OBJECT
public:
    FTPS();
    ~FTPS();
    void sendFiles(QStringList& filesFrom, const QStringList& dirTo, bool createDirNoExists = true);

    void sendFiles(const QVector <QByteArray*>& data, const QStringList& names, const QStringList& dirTo, bool createDirNoExists = true);

    void getFile(const QString& path, const QString& saveTo);

    void getFile(const QString& path, const QByteArray& writeTo);

    QDateTime getFileTime(const QString& path);

    void setConnectionSettings(const QString& username, const QString& password, const QString& url);

    void setReadCallBack(size_t (*ptr)(void*, size_t, size_t, void*)) noexcept
    {read_callback = ptr;}

    void* getReadCallBack() const noexcept
    {return (void*)read_callback;}

    void setWriteCallBack(size_t (*ptr)(void*, size_t, size_t, void*)) noexcept
    {write_callback = ptr;}

    void* getWriteCallBack() const noexcept
    {return (void*)write_callback;}

signals:
    void uploadFinished(QString message);

    void loadFinished();

private:
    size_t (*read_callback)(void*, size_t, size_t, void*);

    size_t (*write_callback)(void *, size_t, size_t, void*);

    void handleSendStillRunning(QTimer* timer, int& stillRunning, CURLM* multiHandle);

    void handleSendDone(QTimer* timer, CURLM* multiHandle, const int handleCount, CURL**handles);

    void handleGet(const QString& path, CURLcode& res, const void* device, CURL *curl);

    void prepareSendHandles(QString& filePath, CURL *handles, bool createDirNoExists, const void* device, curl_off_t fSize);

    QVector <QTimer*> timers;
    QVector <QIODevice*> devices;
    FTPSSettings settings;

};



#endif // FTPS_H
