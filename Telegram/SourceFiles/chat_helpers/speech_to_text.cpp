//
// Created by akhoroshev on 27.03.19.
//
#include "speech_to_text.h"

#include "boxes/abstract_box.h"
#include "data/data_document.h"

#include "google/cloud/speech/v1/cloud_speech.grpc.pb.h"
#include "grpc++/grpc++.h"

#include <QProcess>
#include <QTimer>

#include <fstream>
#include <iostream>

using google::cloud::speech::v1::RecognitionConfig;
using google::cloud::speech::v1::Speech;
using google::cloud::speech::v1::RecognizeRequest;
using google::cloud::speech::v1::RecognizeResponse;


class MockSpeechToText : public SpeechToText {
public:
    void execute(const FullMsgId id, DocumentData *ptr) override {
        //ptr->save(id, "tmp.ogg");

        std::ifstream file("/home/akhoroshev/.local/share/TelegramDesktop/tmp.ogg");
        if (!file) {
            std::cerr << "Fail in openning file" << std::endl;
            return;
        }

        RecognizeRequest request;
        request.mutable_config()->set_language_code("ru");
        request.mutable_config()->set_sample_rate_hertz(48000);
        request.mutable_config()->set_encoding(RecognitionConfig::OGG_OPUS);
        std::cerr << "Assigne..." << std::endl;
        request.mutable_audio()->mutable_content()->assign(
                std::istreambuf_iterator<char>(file.rdbuf()),
                std::istreambuf_iterator<char>());

        std::cerr << "Assigned" << std::endl;

        auto creds = grpc::GoogleDefaultCredentials();

        std::cerr << "GoogleDefaultCredentials" << std::endl;

        auto channel = grpc::CreateChannel("speech.googleapis.com", creds);

        std::cerr << "CreateChannel" << std::endl;

        std::unique_ptr<Speech::Stub> speech(Speech::NewStub(channel));

        std::cerr << "Speech::NewStub(channel)" << std::endl;

        grpc::ClientContext context;
        RecognizeResponse response;
        grpc::Status rpc_status = speech->
                Recognize(&context, request, &response);

        std::cerr << "response" << std::endl;
        std::string msg;
        if (!rpc_status.ok()) {
            msg = "Fail";
            // Report the RPC failure.
        } else {
            msg = "Failure";
            for (int r = 0; r < 1; ++r) {
                auto result = response.results(r);
                for (int a = 0; a < 1; ++a) {
                    auto alternative = result.alternatives(a);
                    msg = alternative.transcript();
                }
            }
        }
        std::cerr << msg << std::endl;
    }


    /*
        QTimer::singleShot(3000, [this]()
        {
            std::ifstream file("/home/akhoroshev/.local/share/TelegramDesktop/tmp.ogg");
            if (!file) {
                std::cerr << "Fail in openning file" << std::endl;
                emit recognized("Fail in openning file");
                deleteLater();
                return;
            }

            RecognizeRequest request;
            request.mutable_config()->set_language_code("ru");
            request.mutable_config()->set_sample_rate_hertz(48000);
            request.mutable_config()->set_encoding(RecognitionConfig::OGG_OPUS);
            std::cerr << "Assigne..." << std::endl;
            request.mutable_audio()->mutable_content()->assign(
                    std::istreambuf_iterator<char>(file.rdbuf()),
                    std::istreambuf_iterator<char>());

            std::cerr << "Assigned" << std::endl;

            auto creds = grpc::GoogleDefaultCredentials();

            std::cerr << "GoogleDefaultCredentials" << std::endl;


            auto channel = grpc::CreateChannel("speech.googleapis.com", creds);

            std::cerr << "CreateChannel" << std::endl;

            std::unique_ptr<Speech::Stub> speech(Speech::NewStub(channel));

            std::cerr << "Speech::NewStub(channel)" << std::endl;

            grpc::ClientContext context;
            RecognizeResponse response;
            grpc::Status rpc_status = speech->
                    Recognize(&context, request, &response);

            std::cerr << "response" << std::endl;

            if (!rpc_status.ok()) {
                // Report the RPC failure.
                emit recognized(QString::fromStdString(rpc_status.error_message()));
            } else {
                QString msg = "Failure";
                for (int r = 0; r < 1; ++r) {
                    auto result = response.results(r);
                    for (int a = 0; a < 1; ++a) {
                        auto alternative = result.alternatives(a);
                        msg = QString::fromStdString(alternative.transcript());
                    }
                }
                emit recognized(msg);
            }
//            emit recognized("Привет Артем");
            deleteLater();
        });
*/

};


SpeechToText *SpeechToText::create_instance() {
    return new MockSpeechToText();
}