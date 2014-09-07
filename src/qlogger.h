/************************************************************************************
 *                                                                                  *
 *  The MIT License (MIT)                                                           *
 *                                                                                  *
 *  Copyright (c) 2014 Daniele                                                      *
 *                                                                                  *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy    *
 *  of this software and associated documentation files (the "Software"), to deal   *
 *  in the Software without restriction, including without limitation the rights    *
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell       *
 *  copies of the Software, and to permit persons to whom the Software is           *
 *  furnished to do so, subject to the following conditions:                        *
 *                                                                                  *
 *  The above copyright notice and this permission notice shall be included in all  *
 *  copies or substantial portions of the Software.                                 *
 *                                                                                  *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR      *
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,        *
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE     *
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER          *
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   *
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE   *
 *  SOFTWARE.                                                                       *
 * **********************************************************************************
*/

#ifndef QLOGGER_H
#define QLOGGER_H

#include "qlogger_global.h"

#include <QStringList>

#include <QThread>
#include <QMutex>
#include <QWaitCondition>

#include <QFile>
#include <QAbstractSocket>

#include <memory>

/*! \mainpage QLogger library
 *  This library provides utilities for writing quickly and thread-safetly
 *  log messages into a stream(socket, file, etc..).
 *
 *  To use this just create a QLogger object and pass it a unique pointer with the
 *  desired stream and then call start on the logger itself.
 *  To stop it just call finishWriting() and then wait().
 *  \author D'Orazio Daniele
 *  \version 0.1
 */

/*!
 *  \class QLoggerStream ""
 *  \brief The QLoggerStream class
 *  It's the interface that needs to be implemented in order
 *  to use QLogger.
 */
class QLOGGERSHARED_EXPORT QLoggerStream
{
public:
    /*!
     *  \brief Destructor
     */
    virtual ~QLoggerStream() {}

    /*!
     *  \brief Opens the stream
     *  \return true if sucessful, otherwise false
     */
    virtual bool open() = 0;

    /*!
     *  \brief checks if the stream is open
     *  \return true if open, otherwise false
     */
    virtual bool isOpen() const = 0;

    /*!
     *  \brief Writes in the stream
     *  \param s string to write
     *  \return bytes actually written or -1 if an error occured
     */
    virtual qint64 write(const QString& s) = 0;

    /*!
     *  \brief Closes the stream
     */
    virtual void close() = 0;

    /*!
     *  \brief error utility
     *  \return the last error description
     */
    virtual QString errorString() const = 0;
};

/*!
 *  \class QLoggerFileStream ""
 *  \brief The QLoggerFileStream class
 *  It's an implementation of QLoggerStream working
 *  on a file.
 */
class QLOGGERSHARED_EXPORT QLoggerFileStream : public QLoggerStream
{
public:
    /*!
     *  \brief QLoggerFileStream
     *  Default constructor
     *  \param filename
     *  \sa filename()
     */
    explicit QLoggerFileStream(const QString& filename = "");

    /*!
     * \brief opens the file
     * \return true if sucessful, otherwise false
     */
    bool open() Q_DECL_OVERRIDE;

    /*!
     *  \brief open utility
     *  \return true if the stream is open, otherwise false
     */
    bool isOpen() const Q_DECL_OVERRIDE;

    /*!
     *  \brief writes s in the file
     *  \param s string to write
     *  \return bytes actually written
     */
    qint64 write(const QString& s) Q_DECL_OVERRIDE;

    /*!
     *  \brief flushes the stream
     *  \return true if successful otherwise false
     */
    bool flush();

    /*!
     *  \brief closes the stream
     */
    void close() Q_DECL_OVERRIDE;

    /*!
     *  \brief error utility
     *  \return the last error description
     */
    QString errorString() const Q_DECL_OVERRIDE;

    /*!
     *  \brief setter
     *  \param filename
     */
    void setFilename(const QString& filename);

    /*!
     *  \brief getter
     *  \return the current filename
     */
    QString filename() const;

    /*!
     *  \brief setter
     *  \param rate a negative value means never
     *  \sa flushRate()
     */
    void setFlushRate(int rate);

    /*!
     *  \brief getter
     *  \return flush rate
     *  \sa setFlushRate()
     */
    int flushRate() const;
private:
    QFile   _file;  /*!< log file */

    int     _flush_rate;    /*!< flush rate, a negative value means never; default is 4 \sa flush() */
    int     _flush_count;   /*!< flush counter \sa flushRate()*/
};

/*!
 *  \class QLoggerSocketStream ""
 *  \brief The QLoggerSocketStream class
 *  It's an implementation of QLoggerStream working on a socket.
 *  In order to work the socket must have no parent(parent() should return 0)
 *  and it must have the thread affinity of the proper logger.
 *  To build a server with
 *  <a href= "https://qt-project.org/doc/qt-4.8/qtcpserver.html"> QTcpServer </a>
 *  you should subclass it and reimplement
 *  <a href= "https://qt-project.org/doc/qt-4.8/qtcpserver.html#incomingConnection"> incomingConnection()</a>,
 *  because
 *  <a href= "https://qt-project.org/doc/qt-4.8/qtcpserver.html#nextPendingConnection">n extPendingConnection</a>
 *  doesn't work for sockets in different threads.
 */
class QLOGGERSHARED_EXPORT QLoggerSocketStream : public QLoggerStream
{
public:
    using socket_ptr = std::unique_ptr<QAbstractSocket>;    //!< pointer type for the socket

    /*!
     *  \brief QLoggerSocketStream
     *  Default constructor
     *  \param hostname
     *  \param port
     *  \param socketImpl implementation of the socket used (QTcpSocket, QSslsocket...)
     *  \sa hostname() port() socket()
     */
    explicit QLoggerSocketStream(socket_ptr socketImpl,
            const QString& hostname = QString(),
            quint16 port = 0);

    /*!
     *  \brief setter
     *  \param hostname the socket will connect to
     *  \sa hostname()
     */
    void setHostname(const QString& hostname);

    /*!
     *  \brief getter
     *  \return hostname
     *  \sa setHostname()
     */
    QString hostname() const;

    /*!
     *  \brief setter
     *  \param port the socket will connect to
     */
    void setPort(quint16 port);

    /*!
     *  \brief getter
     *  \return port
     *  \sa setPort()
     */
    quint16 port() const;

    /*!
     *  \brief getter
     *  \return the socket used in the connection
     */
    QAbstractSocket* socket() const;

    /*!
     *  \brief open the socket and waits until a connection is established
     *  In case of ssl socket it will call
     *  <a href= "https://qt-project.org/doc/qt-4.8/qsslsocket.html#connectToHostEncrypted">
     *  connectToHostEncrypted()</a>, otherwise
     *  <a href = "https://qt-project.org/doc/qt-4.8/qabstractsocket.html#connectToHost">
     *  connectToHost() </a>
     *  \return true if success, otherwise false
     */
    bool open() Q_DECL_OVERRIDE;

    /*!
     *  \brief open utility
     *  \return true if the stream is open, otherwise false
     */
    bool isOpen() const Q_DECL_OVERRIDE;

    /*!
     *  \brief writes s into the stream
     *  \param s string to write
     *  \return payload written
     */
    qint64 write(const QString &s) Q_DECL_OVERRIDE;

    /*!
      * \brief closes the stream and waits until socket it's disconnected
      */
    void close() Q_DECL_OVERRIDE;

    /*!
     *  \brief error utility
     *  \return the last error description
     */
    QString errorString() const Q_DECL_OVERRIDE;
private:
    socket_ptr  _socket;            //!< socket to use, his parent is reset in the constructor to nullptr

    QString     _hostname;          //!< hostname the socket will connect to
    quint16     _port;              //!< port the socket will connect to
};

/*!
 *  \class QLoggerDebugStream ""
 *  \brief The QLoggerDebugStream class
 *  It's an implementation of QLoggerStream working on qDebug()
 */
class QLOGGERSHARED_EXPORT QLoggerDebugStream : public QLoggerStream
{
public:
    /*!
     *  \brief opens the stream
     *  \return always true
     */
    bool open() Q_DECL_OVERRIDE { return true; }

    /*!
     *  \brief checks if the stream is open
     *  \return always true
     */
    bool isOpen() const Q_DECL_OVERRIDE { return true; }

    /*!
     *  \brief Writes in the stream
     *  \param s string to write
     *  \return always 42
     */
    qint64 write(const QString &s) Q_DECL_OVERRIDE { qDebug() << s; return 42; }

    /*!
     *  \brief Closes the stream
     */
    void close() Q_DECL_OVERRIDE {}

    /*!
     *  \brief error utility
     *  \return always ""
     */
    QString errorString() const Q_DECL_OVERRIDE { return "";}
};

/*!
 *  \class QLogger ""
 *  \brief The QLogger class
 *  It's a thread working as a basic logger.
 *  Moreover it's extremely easy to use and it's completely thread-safe.
 *
 *  QLogger can only manage a file set in the constructor and cannot be changed.
 *  After have set filename, just add a message with its level with addMessage().
 *  Finally to terminate properly, first call finishWriting() and then
 *  <a href = "http://qt-project.org/doc/qt-4.8/qthread.html#wait">QThread::wait()</a>
 *  on the thread object.
 *
 *  The format of the log message is [datetime] logLevel message. However it's highly
 *  customizable: just subclass QLogger and then reiplement logLevelToString(), datetimeFormat()
 *  and messageFormat().
 *
 *  To check if an error occured use errorString().
 *
 *  Here it is a basic use of the QLogger class.
 * \code
 *     QLogger logger(QLogger::stream_ptr(new QLoggerFileStream("test.log")));
 *
 *     QString message;
 *     for (int i = 0; i < 10000; ++i)
 *          message += QString::number(i);
 *
 *      logger.addMessage(message, QLogger::LogLevel::Info);
 *      logger.start(QThread::IdlePriority);
 *
 *      logger.addMessage(message, QLogger::LogLevel::Fatal);
 *
 *      logger.finishWriting();
 *      logger.wait();
 *
 *      if (!logger.errorString().isEmpty())
 *          qDebug() << logger.errorString();
 * \endcode
 *
 *  Note: debugging options are by default enabled. To disable them uncomment
 *  a line in the pro file.
 *
 */
class QLOGGERSHARED_EXPORT QLogger : public QThread
{
public:
    using stream_ptr = std::unique_ptr<QLoggerStream>;      //!< alias for std::unique_ptr<QLoggerStream>

    /*!
     *  \brief The LogLevel enum
     *  Contains the basic log levels the can be used when logging a message
     *  \sa addMessage()
     */
    enum class LogLevel {
        Info = 0,       //!< Info message
        Debug,          //!< Debugging message
        Warning,        //!< Warning message
        Fatal           //!< Fatal message, very dangerous
    };

    /*!
     *  \brief Default constructor
     *  \param stream the stream to use
     *  \param parent the parent of the logger if any
     */
    explicit QLogger(stream_ptr stream, QObject * parent = 0);

    /*!
     *  \brief Destructor
     *  \sa QLogger()
     */
    virtual ~QLogger();

    /*!
     *  \brief getter
     *  \return a reference to the stream
     *  \sa _stream
     */
    stream_ptr& stream();

    /*!
     *  \brief getter
     *  \return a copy of the messages that have to be written
     *  \sa addMessage()
     */
    QStringList messages() const;

    /*!
     *  \brief To use when checking for errors.
     *  Use this after the entire work of the QLogger thread.
     *  \return the description of the last error
     */
    QString errorString() const;

    /*!
     *  \brief Returns the format for the message
     *  This must return a string in arg form, e.g.("[%1] %2 %3") that represents
     *  the format for the messages. Note that by %1 is datetime, %2 is the log level
     *  and %3 is the message body.
     *  \return the format of the message
     *  \sa logLevelToString(), datetimeFormat()
     */
    QString formatString() const;

    /*!
     *  \brief Returns the datetime format to be used.
     *  It can be reimplemented by subclasses.
     *  \return the string containg the datetime format
     */
    QString datetimeFormat() const;
public slots:
    /*!
     *  \brief Adds a message to the list
     *  \param message
     *  \param level
     *  \sa messages()
     */
    void addMessage(const QString& message, const LogLevel& level);

    /*!
     *  \brief Tells the thread to finish writing its messages and
     *          then to terminate.
     *  This function should be always used before calling
     *  <a href = "http://qt-project.org/doc/qt-4.8/qthread.html#wait">QThread::wait()</a>
     *  in order
     *  to wait the logger until it has finished
     */
    void finishWriting();

    /*!
     *  \brief setFormatString
     *  \param formatString
     *  \sa formatString()
     */
    void setFormatString(const QString& formatString);

    /*!
     *  \brief setDatetimeFormat
     *  \param datetimeFormat
     *  \sa datetimeFormat()
     */
    void setDatetimeFormat(const QString& datetimeFormat);
protected:
    /*!
      * \brief Run method reimplemented from <a href = "http://qt-project.org/doc/qt-4.8/qthread.html#run">run()</a>
      */
    void run() Q_DECL_OVERRIDE;

    /*!
     *  \brief Transform a log level in a readable format
     *  It can be reimplemented by subclasses.
     *  \param level log level to translate
     *  \return the formatted string
     */
    virtual QString logLevelToString(const LogLevel& level) const;
private:
    stream_ptr          _stream;        /*!< stream to use for writing the messages \sa _messages */
    QStringList         _messages;      /*!< messages to write \sa messages(), addMessage() */

    mutable QMutex      _mutex;         //!< mutex to synchronize threads
    QWaitCondition      _empty;         //!< allows to wait while there aren't messages to be written
    QAtomicInt          _finish;        //! if set to true, it tells that when the thread will have written all the messages,
                                        //! then it will stop
    QAtomicInt          _messages_size; //!< atomic int to ensure thread-safety. It's equal of _messages.size()

    QString             _error_string;  //!< description of the last error

    QString             _format_string;     //!< format of the message \sa formatString()
    QString             _datetime_format;   //!< datetime format to be used \sa datetimeFormat()
};

#endif // QLOGGER_H
