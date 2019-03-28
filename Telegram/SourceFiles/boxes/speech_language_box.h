/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "lang/lang_cloud_manager.h"
#include "boxes/abstract_box.h"
#include "base/binary_guard.h"
#include "data/data_document.h"

namespace Ui {
class MultiSelect;
struct ScrollToRequest;
} // namespace Ui

class SpeechLanguageBox : public BoxContent {
public:
	SpeechLanguageBox(QWidget* w, FullMsgId itemId, DocumentData *data) : _itemId(itemId), _data(data) {
	}

	void setInnerFocus() override;

	static base::binary_guard Show(FullMsgId, DocumentData*);

protected:
	void prepare() override;

	void keyPressEvent(QKeyEvent *e) override;

private:
    FullMsgId _itemId;
    DocumentData* _data;

	using Languages = Lang::CloudManager::Languages;

	not_null<Ui::MultiSelect*> createMultiSelect();
	int rowsInPage() const;

	Fn<void()> _setInnerFocus;
	Fn<Ui::ScrollToRequest(int rows)> _jump;

    void openAndCloseBox(const QString language);
};
