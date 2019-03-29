//
// Created by akhoroshev on 27.03.19.
//

#include "speech_to_text.h"


#include <QProcess>
#include <QTimer>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include "data/data_document.h"
#include "boxes/abstract_box.h"
#include "storage/file_download.h"

#include <iostream>

const char *errorMsg = "Ошибка";

class MockSpeechToText : public SpeechToText {
public:
    MockSpeechToText() : apiKey(qgetenv("GOOGLE_APPLICATION_API_KEY")) {}

    void execute(const FullMsgId id, DocumentData *ptr, QString lang) override {
        if (apiKey.isEmpty()) {
            emit recognized("Api key not found");
            deleteLater();
            return;
        }

        ptr->save(id, "tested.ogg");

        QTimer::singleShot(3000, [=](){
            QFile file("tested.ogg");
            bool fileOpenStatus = file.open(QIODevice::ReadOnly);
            if (!fileOpenStatus) {
                emit recognized(errorMsg);
                deleteLater();
                return;
            }
            QByteArray byteArray = file.readAll();
            onData(byteArray, lang);
        });


//        if (ptr->data().isEmpty()) {
//            qDebug() << "Empty data";
//            // trigger creating loader process
//            ptr->save(id, "tested.ogg");
//            if (!ptr->getFileLoader()) {
//                // TODO: fix
//                emit recognized(errorMsg);
//                deleteLater();
//            } else {
//                QObject::connect(ptr->getFileLoader(), &FileLoader::progress, this, [=](FileLoader *fileLoader) {
//                    if (fileLoader->finished()) {
//                        onData(ptr->data(), lang);
//                    }
//                });
//            }
//        } else {
//            onData(ptr->data(), lang);
//        }
    }

private:
    void onData(QByteArray byteArray, QString lang) {
        if (byteArray.isEmpty() || byteArray.size() < 44) {
            emit recognized(errorMsg);
            deleteLater();
            return;
        }

        uint32_t sample_rate_hertz = *reinterpret_cast<uint32_t *>(byteArray.data() + 40);
        if (sample_rate_hertz != 8000 && sample_rate_hertz != 12000 &&
            sample_rate_hertz != 16000 && sample_rate_hertz != 24000 &&
            sample_rate_hertz != 48000) {
            emit recognized(errorMsg);
            deleteLater();
            return;
        }

        QString jsonData = byteArray.toBase64();
        makeRequest(jsonData, sample_rate_hertz, lang);
    }


    void makeRequest(QString &jsonData, uint32_t sample_rate_hertz, QString lang) {
        QObject::connect(&qNetworkAccessManager, &QNetworkAccessManager::finished, this, &MockSpeechToText::handler);

        QNetworkRequest request(
                QUrl("https://speech.googleapis.com/v1/speech:recognize?key=" + apiKey));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
        QJsonObject jsonObject;

        QJsonObject configObject;
        configObject.insert("encoding", "OGG_OPUS");
        configObject.insert("sampleRateHertz", (qint64) sample_rate_hertz);

        qDebug() << "lang:" << lang;

        if (lang == "English" || lang == "en") {
            configObject.insert("languageCode", "en-US");
        } else {
            //TODO: all languages support
            configObject.insert("languageCode", "ru-RU");
        }

        jsonObject.insert("config", configObject);

        QJsonObject audioObject;
        audioObject.insert("content", jsonData);

        jsonObject.insert("audio", audioObject);

        QJsonDocument jsonDoc(jsonObject);
        QByteArray jsonBytes = jsonDoc.toJson();
        qNetworkAccessManager.post(request, jsonBytes);
    }


    void handler(QNetworkReply *reply) {
        QString msg;

        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            QJsonDocument document = QJsonDocument::fromJson(responseData);
            QJsonObject jsonObject = document.object();

            if (jsonObject.contains("results")) {
                QJsonValue lel = jsonObject["results"].toArray().at(0).toObject()["alternatives"].toArray().at(
                        0).toObject()["transcript"];
                msg = lel.toString();
            }
        }
        emit recognized(msg.size() ? msg : errorMsg);
        deleteLater();
        reply->deleteLater();
    }


    QNetworkAccessManager qNetworkAccessManager;
    QString apiKey;
};


SpeechToText *SpeechToText::create_instance() {
    return new MockSpeechToText();
}
