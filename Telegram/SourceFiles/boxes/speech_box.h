//
// Created by fatall on 27.03.19.
//

#ifndef TELEGRAM_SPEECH_BOX_H
#define TELEGRAM_SPEECH_BOX_H

#include "boxes/abstract_box.h"

namespace Ui {
    class LinkButton;
    class FlatLabel;
} // namespace Ui

class SpeechBox : public BoxContent {
public:
    SpeechBox(QWidget*, QString text);

protected:
    void prepare() override;

    void resizeEvent(QResizeEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;

private:
    object_ptr<Ui::FlatLabel> _text;

};


#endif //TELEGRAM_SPEECH_BOX_H
