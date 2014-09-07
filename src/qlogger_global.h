#ifndef QLOGGER_GLOBAL_H
#define QLOGGER_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(QLOGGER_LIBRARY)
#  define QLOGGERSHARED_EXPORT Q_DECL_EXPORT
#else
#  define QLOGGERSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // QLOGGER_GLOBAL_H
