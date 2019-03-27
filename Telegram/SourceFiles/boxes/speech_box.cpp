#include "boxes/speech_box.h"

#include "lang/lang_keys.h"
#include "mainwidget.h"
#include "mainwindow.h"
#include "boxes/confirm_box.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "styles/style_boxes.h"
#include "platform/platform_file_utilities.h"
#include "core/click_handler_types.h"
#include "core/update_checker.h"

SpeechBox::SpeechBox(QWidget *parent, QString text)
        : _text(this, text, Ui::FlatLabel::InitType::Rich, st::aboutLabel) {
}

void SpeechBox::prepare() {
    setTitle([] { return qsl("Speech to text"); });

    addButton(langFactory(lng_close), [this] { closeBox(); });

    const auto linkFilter = [](const ClickHandlerPtr &link, auto button) {
        if (const auto url = dynamic_cast<UrlClickHandler*>(link.get())) {
            url->UrlClickHandler::onClick({ button });
            return false;
        }
        return true;
    };

    _text->setClickHandlerFilter(linkFilter);

    setDimensions(st::aboutWidth, _text->height());
}

void SpeechBox::resizeEvent(QResizeEvent *e) {
    BoxContent::resizeEvent(e);

    _text->moveToLeft(st::boxPadding.left(), 0);
}


void SpeechBox::keyPressEvent(QKeyEvent *e) {
    if (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return) {
        closeBox();
    } else {
        BoxContent::keyPressEvent(e);
    }
}
