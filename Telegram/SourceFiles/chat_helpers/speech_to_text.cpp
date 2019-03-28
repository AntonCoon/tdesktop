//
// Created by akhoroshev on 27.03.19.
//

#include "speech_to_text.h"

#include <QProcess>
#include <QTimer>

#include "data/data_document.h"
#include "boxes/abstract_box.h"


class MockSpeechToText : public SpeechToText
{
public:
    void execute(const FullMsgId id, DocumentData *ptr, QString lang) override
    {
        ptr->save(id, "tmp.ogg");

        QTimer::singleShot(1000, [this]()
        {
            QProcess process;
            process.start("python3", QStringList() << "script.py" << "--input" << "tmp.ogg");
            process.waitForFinished();
            emit recognized(process.readAllStandardOutput());
            deleteLater();
        });
    }
};


SpeechToText *SpeechToText::create_instance()
{
    return new MockSpeechToText();
}
