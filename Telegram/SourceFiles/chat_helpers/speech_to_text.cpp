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
        ptr->save(id, "xxx");

        QTimer::singleShot(1000, [this]()
        {
            emit recognized("Привет Артем");
            deleteLater();
        });
    }
};


SpeechToText *SpeechToText::create_instance()
{
    return new MockSpeechToText();
}
