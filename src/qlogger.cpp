#include <QDateTime>
#include <QMutexLocker>
#include <QSslSocket>

#include "qlogger.h"

#include <QDebug>

QLoggerFileStream::QLoggerFileStream(const QString &filename) :
    QLoggerStream(), _file(filename), _flush_rate(4), _flush_count(0) {}

bool QLoggerFileStream::open()
{
    return _file.open(QIODevice::Append | QIODevice::Text);
}

bool QLoggerFileStream::isOpen() const
{
    return _file.isOpen();
}

qint64 QLoggerFileStream::write(const QString &s)
{
    qint64 bytes = _file.write(s.toUtf8());

    if (bytes != -1 && _flush_rate > 0) {
        _flush_count = (_flush_count + 1) % _flush_rate;
        if (_flush_count == 0)
            _file.flush();
    }

    return bytes;
}

bool QLoggerFileStream::flush()
{
    return _file.flush();
}

void QLoggerFileStream::close()
{
    _file.close();
}

QString QLoggerFileStream::errorString() const
{
    return _file.errorString();
}

void QLoggerFileStream::setFilename(const QString &filename)
{
    _file.setFileName(filename);
}

QString QLoggerFileStream::filename() const
{
    return _file.fileName();
}

void QLoggerFileStream::setFlushRate(int rate)
{
    _flush_rate = rate;
    _flush_count = 0;
}

int QLoggerFileStream::flushRate() const
{
    return _flush_rate;
}

QLoggerSocketStream::QLoggerSocketStream(socket_ptr socketImpl, const QString &hostname,
                                         quint16 port) :
    QLoggerStream(), _socket(std::move(socketImpl)), _hostname(hostname), _port(port)
{
}

void QLoggerSocketStream::setHostname(const QString &hostname)
{
    _hostname = hostname;
}

QString QLoggerSocketStream::hostname() const
{
    return _hostname;
}

void QLoggerSocketStream::setPort(quint16 port)
{
    _port = port;
}

quint16 QLoggerSocketStream::port() const
{
    return _port;
}

QAbstractSocket *QLoggerSocketStream::socket() const
{
    return _socket.get();
}

bool QLoggerSocketStream::open()
{
    QSslSocket* ssl_socket = qobject_cast<QSslSocket*>(_socket.get());
    if (ssl_socket != nullptr) {
        ssl_socket->connectToHostEncrypted(_hostname, _port, QIODevice::WriteOnly);
    }
    else {
        _socket->connectToHost(_hostname, _port, QIODevice::WriteOnly);
    }

    return _socket->waitForConnected();
}

bool QLoggerSocketStream::isOpen() const
{
    return _socket->isOpen();
}

qint64 QLoggerSocketStream::write(const QString &s)
{
    qint64 bytes = _socket->write(s.toUtf8());
    _socket->waitForBytesWritten();
    return bytes;
}

void QLoggerSocketStream::close()
{
    _socket->disconnectFromHost();
}

QString QLoggerSocketStream::errorString() const
{
    return _socket->errorString();
}

QLogger::QLogger(stream_ptr stream, QObject *parent) :
    QThread(parent), _stream(std::move(stream))
{
    _finish.store(0);
    _messages_size.store(0);

    _error_string   = "";
    _format_string  = "[%1] %2 %3";
    _datetime_format= "dd.MM.yyyy hh:mm:ss";
}

QLogger::~QLogger()
{
    if (_stream->isOpen())
        _stream->close();   // RAII
}

QLogger::stream_ptr& QLogger::stream()
{
    return _stream;
}

void QLogger::addMessage(const QString &message, const LogLevel &level)
{
    qDebug() << "QLogger::addMessage()";

    _empty.wakeOne();
    {
        QMutexLocker locker(&_mutex);
        const QString levelString = logLevelToString(level);

        const QString datetime = QDateTime::currentDateTime().toString(_datetime_format);

        _messages.append(_format_string.arg(datetime).arg(levelString).arg(message) + "\n");
        _messages_size.store(_messages.size());
    }
    qDebug() << "QLogger::addMessage----->Wake one";
}

void QLogger::run()
{
    if (!_stream->open()) {
        _error_string = _stream->errorString();
        _stream->close();
        return;
    }

    while ( !_finish.load() || _messages_size.load() != 0) {
        if (_messages_size.load() != 0) {
            qDebug() << "QLogger::run()----->Mutex lock";
            _mutex.lock();
            const QString message = _messages.front();
            _messages.pop_front();
            _messages_size.store(_messages.size());

            _mutex.unlock();

            qDebug() << "QLogger::run()----->Mutex unlock";

            qDebug() << "QLogger::run()----->Stream writing";
            _stream->write(message);
        }
        else if (!_finish.load()){
            qDebug() << "QLogger::run()----->Must wait";
            _mutex.lock();
            _empty.wait(&_mutex);
            _mutex.unlock();
        }

    }
    _stream->close();
    qDebug() << "QLogger::run()----->End run";
}

void QLogger::finishWriting()
{
    _empty.wakeOne();       // it could be waiting
    QMutexLocker locker(&_mutex);
    _finish = 1;
    qDebug() << "QLogger::finishWriting()----->Wake one";
}

QString QLogger::logLevelToString(const LogLevel &level) const
{
    switch (level) {
    case LogLevel::Info:        return "INFO";
    case LogLevel::Debug:       return "DEBUG";
    case LogLevel::Warning:     return "WARNING";
    case LogLevel::Fatal:       return "FATAL";
    }

    return "";
}

QStringList QLogger::messages() const
{
    QMutexLocker locker(&_mutex);
    return _messages;
}

QString QLogger::errorString() const
{
    QMutexLocker locker(&_mutex);
    return _error_string;
}

QString QLogger::formatString() const
{
    QMutexLocker locker(&_mutex);
    return _format_string;
}

QString QLogger::datetimeFormat() const
{
    QMutexLocker locker(&_mutex);
    return _datetime_format;
}

void QLogger::setFormatString(const QString &formatString)
{
    QMutexLocker locker(&_mutex);
    _format_string = formatString;
}

void QLogger::setDatetimeFormat(const QString &datetimeFormat)
{
    QMutexLocker locker(&_mutex);
    _datetime_format = datetimeFormat;
}
