//logging.h
#pragma once

#include <QMessageLogContext>
#include <QString>

void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);