//schedule.h
#pragma once

#include <QObject>
#include <QString>
#include <QPair>
#include <QStringList>

class CSchedule : public QObject 
{
    Q_OBJECT

//Constructor
public:
    explicit CSchedule(QObject* parent = nullptr);
	~CSchedule();

//Methods
    public:
    bool getNextInstruction( QString& action, QString& detail );
    bool readInstructionFile(const QString instructionFilename );

    private:
    QPair<QString, QString> interpretInstruction(QString& fullInstruction );
    bool validateKey(const QString& key);

//Member variables
private:
    QList<QPair<QString, QString>> m_instructions;
    QList<QPair<QString,QString>>::iterator m_iterator;
    static const QList<QString>  m_validKeys;
};