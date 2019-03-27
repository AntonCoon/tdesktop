//
// Created by akhoroshev on 27.03.19.
//

#ifndef TELEGRAM_SPEECHTOTEXT_H
#define TELEGRAM_SPEECHTOTEXT_H

#include <QObject>
#include "data/data_document.h"

class SpeechToText : public QObject {
    Q_OBJECT
public:
    virtual void execute(FullMsgId id, DocumentData* ptr) = 0;

signals:
    void recognized(QString message);
};


#endif //TELEGRAM_SPEECHTOTEXT_H
