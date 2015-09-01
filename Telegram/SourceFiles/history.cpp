/*
This file is part of Telegram Desktop,
the official desktop version of Telegram messaging app, see https://telegram.org

Telegram Desktop is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

It is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

Full license: https://github.com/telegramdesktop/tdesktop/blob/master/LICENSE
Copyright (c) 2014 John Preston, https://desktop.telegram.org
*/
#include "stdafx.h"
#include "style.h"
#include "lang.h"

#include "history.h"
#include "mainwidget.h"
#include "application.h"
#include "fileuploader.h"
#include "window.h"
#include "gui/filedialog.h"

#include "audio.h"
#include "localstorage.h"

TextParseOptions _textNameOptions = {
	0, // flags
	4096, // maxw
	1, // maxh
	Qt::LayoutDirectionAuto, // lang-dependent
};
TextParseOptions _textDlgOptions = {
	0, // flags
	0, // maxw is style-dependent
	1, // maxh
	Qt::LayoutDirectionAuto, // lang-dependent
};
TextParseOptions _historyTextOptions = {
	TextParseLinks | TextParseMentions | TextParseHashtags | TextParseMultiline | TextParseRichText, // flags
	0, // maxw
	0, // maxh
	Qt::LayoutDirectionAuto, // dir
};
TextParseOptions _historyBotOptions = {
	TextParseLinks | TextParseMentions | TextParseHashtags | TextParseBotCommands | TextParseMultiline | TextParseRichText, // flags
	0, // maxw
	0, // maxh
	Qt::LayoutDirectionAuto, // dir
};

namespace {
	TextParseOptions _historySrvOptions = {
		TextParseLinks | TextParseMentions | TextParseHashtags | TextParseMultiline | TextParseRichText, // flags
		0, // maxw
		0, // maxh
		Qt::LayoutDirectionAuto, // lang-dependent
	};
	TextParseOptions _webpageTitleOptions = {
		TextParseMultiline | TextParseRichText, // flags
		0, // maxw
		0, // maxh
		Qt::LayoutDirectionAuto, // dir
	};
	TextParseOptions _webpageDescriptionOptions = {
		TextParseLinks | TextParseMultiline | TextParseRichText, // flags
		0, // maxw
		0, // maxh
		Qt::LayoutDirectionAuto, // dir
	};
	TextParseOptions _twitterDescriptionOptions = {
		TextParseLinks | TextParseMentions | TextTwitterMentions | TextParseHashtags | TextTwitterHashtags | TextParseMultiline | TextParseRichText, // flags
		0, // maxw
		0, // maxh
		Qt::LayoutDirectionAuto, // dir
	};
	TextParseOptions _instagramDescriptionOptions = {
		TextParseLinks | TextParseMentions | TextInstagramMentions | TextParseHashtags | TextInstagramHashtags | TextParseMultiline | TextParseRichText, // flags
		0, // maxw
		0, // maxh
		Qt::LayoutDirectionAuto, // dir
	};

	inline void _initTextOptions() {
		_historySrvOptions.dir = _textNameOptions.dir = _textDlgOptions.dir = cLangDir();
		_textDlgOptions.maxw = st::dlgMaxWidth * 2;
		_webpageTitleOptions.maxw = st::msgMaxWidth - st::msgPadding.left() - st::msgPadding.right() - st::webPageLeft;
		_webpageTitleOptions.maxh = st::webPageTitleFont->height * 2;
		_webpageDescriptionOptions.maxw = st::msgMaxWidth - st::msgPadding.left() - st::msgPadding.right() - st::webPageLeft;
		_webpageDescriptionOptions.maxh = st::webPageDescriptionFont->height * 3;
	}

	AnimatedGif animated;

	inline HistoryReply *toHistoryReply(HistoryItem *item) {
		return item ? item->toHistoryReply() : 0;
	}
	inline const HistoryReply *toHistoryReply(const HistoryItem *item) {
		return item ? item->toHistoryReply() : 0;
	}
	inline HistoryForwarded *toHistoryForwarded(HistoryItem *item) {
		return item ? item->toHistoryForwarded() : 0;
	}
	inline const HistoryForwarded *toHistoryForwarded(const HistoryItem *item) {
		return item ? item->toHistoryForwarded() : 0;
	}
	inline const TextParseOptions &itemTextParseOptions(HistoryItem *item) {
		return itemTextParseOptions(item->history(), item->from());
	}
}

const TextParseOptions &itemTextParseOptions(History *h, UserData *f) {
	if ((!h->peer->chat && h->peer->asUser()->botInfo) || (!f->chat && f->asUser()->botInfo) || (h->peer->chat && h->peer->asChat()->botStatus >= 0)) {
		return _historyBotOptions;
	}
	return _historyTextOptions;
}

void historyInit() {
	_initTextOptions();
}

void startGif(HistoryItem *row, const QString &file) {
	if (row == animated.msg) {
		stopGif();
	} else {
		animated.start(row, file);
	}
}

void itemReplacedGif(HistoryItem *oldItem, HistoryItem *newItem) {
	if (oldItem == animated.msg) {
		animated.msg = newItem;
	}
}

void itemRemovedGif(HistoryItem *item) {
	if (item == animated.msg) {
		animated.stop(true);
	}
}

void stopGif() {
	animated.stop();
}

void DialogRow::paint(QPainter &p, int32 w, bool act, bool sel) const {
	QRect fullRect(0, 0, w, st::dlgHeight);
	p.fillRect(fullRect, (act ? st::dlgActiveBG : (sel ? st::dlgHoverBG : st::dlgBG))->b);
	
	p.drawPixmap(st::dlgPaddingHor, st::dlgPaddingVer, history->peer->photo->pix(st::dlgPhotoSize));

	int32 nameleft = st::dlgPaddingHor + st::dlgPhotoSize + st::dlgPhotoPadding;
	int32 namewidth = w - nameleft - st::dlgPaddingHor;
	QRect rectForName(nameleft, st::dlgPaddingVer + st::dlgNameTop, namewidth, st::msgNameFont->height);

	// draw chat icon
	if (history->peer->chat) {
		p.drawPixmap(QPoint(rectForName.left() + st::dlgChatImgLeft, rectForName.top() + st::dlgChatImgTop), App::sprite(), (act ? st::dlgActiveChatImg : st::dlgChatImg));
		rectForName.setLeft(rectForName.left() + st::dlgChatImgSkip);
	}

	HistoryItem *last = history->lastMsg;
	if (!last) {
		p.setFont(st::dlgHistFont->f);
		p.setPen((act ? st::dlgActiveColor : st::dlgSystemColor)->p);
		if (history->typing.isEmpty() && history->sendActions.isEmpty()) {
			p.drawText(nameleft, st::dlgPaddingVer + st::dlgFont->height + st::dlgFont->ascent + st::dlgSep, lang(lng_empty_history));
		} else {
			history->typingText.drawElided(p, nameleft, st::dlgPaddingVer + st::dlgFont->height + st::dlgSep, namewidth);
		}
	} else {
		// draw date
		QDateTime now(QDateTime::currentDateTime()), lastTime(last->date);
		QDate nowDate(now.date()), lastDate(lastTime.date());
		QString dt;
		if (lastDate == nowDate) {
			dt = lastTime.toString(cTimeFormat());
		} else if (lastDate.year() == nowDate.year() && lastDate.weekNumber() == nowDate.weekNumber()) {
			dt = langDayOfWeek(lastDate);
		} else {
			dt = lastDate.toString(qsl("d.MM.yy"));
		}
		int32 dtWidth = st::dlgDateFont->m.width(dt);
		rectForName.setWidth(rectForName.width() - dtWidth - st::dlgDateSkip);
		p.setFont(st::dlgDateFont->f);
		p.setPen((act ? st::dlgActiveDateColor : st::dlgDateColor)->p);
		p.drawText(rectForName.left() + rectForName.width() + st::dlgDateSkip, rectForName.top() + st::msgNameFont->height - st::msgDateFont->descent, dt);

		// draw check
		if (last->out() && last->needCheck()) {
			const style::sprite *check;
			if (last->id > 0) {
				if (last->unread()) {
					check = act ? &st::dlgActiveCheckImg : &st::dlgCheckImg;
				} else {
					check = act ? &st::dlgActiveDblCheckImg: &st::dlgDblCheckImg;
				}
			} else {
				check = act ? &st::dlgActiveSendImg : &st::dlgSendImg;
			}
			rectForName.setWidth(rectForName.width() - check->pxWidth() - st::dlgCheckSkip);
			p.drawPixmap(QPoint(rectForName.left() + rectForName.width() + st::dlgCheckLeft, rectForName.top() + st::dlgCheckTop), App::sprite(), *check);
		}

		// draw unread
		int32 lastWidth = namewidth, unread = history->unreadCount;
		if (unread) {
			QString unreadStr = QString::number(unread);
			int32 unreadWidth = st::dlgUnreadFont->m.width(unreadStr);
			int32 unreadRectWidth = unreadWidth + 2 * st::dlgUnreadPaddingHor;
			int32 unreadRectHeight = st::dlgUnreadFont->height + 2 * st::dlgUnreadPaddingVer;
			int32 unreadRectLeft = w - st::dlgPaddingHor - unreadRectWidth;
			int32 unreadRectTop = st::dlgHeight - st::dlgPaddingVer - unreadRectHeight;
			lastWidth -= unreadRectWidth + st::dlgUnreadPaddingHor;
			p.setBrush((act ? st::dlgActiveUnreadBG : (history->mute ? st::dlgUnreadMutedBG : st::dlgUnreadBG))->b);
			p.setPen(Qt::NoPen);
			p.drawRoundedRect(unreadRectLeft, unreadRectTop, unreadRectWidth, unreadRectHeight, st::dlgUnreadRadius, st::dlgUnreadRadius);
			p.setFont(st::dlgUnreadFont->f);
			p.setPen((act ? st::dlgActiveUnreadColor : st::dlgUnreadColor)->p);
			p.drawText(unreadRectLeft + st::dlgUnreadPaddingHor, unreadRectTop + st::dlgUnreadPaddingVer + st::dlgUnreadFont->ascent, unreadStr);
		}
		if (history->typing.isEmpty() && history->sendActions.isEmpty()) {
			last->drawInDialog(p, QRect(nameleft, st::dlgPaddingVer + st::dlgFont->height + st::dlgSep, lastWidth, st::dlgFont->height), act, history->textCachedFor, history->lastItemTextCache);
		} else {
			p.setPen((act ? st::dlgActiveColor : st::dlgSystemColor)->p);
			history->typingText.drawElided(p, nameleft, st::dlgPaddingVer + st::dlgFont->height + st::dlgSep, lastWidth);
		}
	}

	p.setPen((act ? st::dlgActiveColor : st::dlgNameColor)->p);
	history->nameText.drawElided(p, rectForName.left(), rectForName.top(), rectForName.width());
}

void FakeDialogRow::paint(QPainter &p, int32 w, bool act, bool sel) const {
	QRect fullRect(0, 0, w, st::dlgHeight);
	p.fillRect(fullRect, (act ? st::dlgActiveBG : (sel ? st::dlgHoverBG : st::dlgBG))->b);

	History *history = _item->history();

	p.drawPixmap(st::dlgPaddingHor, st::dlgPaddingVer, history->peer->photo->pix(st::dlgPhotoSize));

	int32 nameleft = st::dlgPaddingHor + st::dlgPhotoSize + st::dlgPhotoPadding;
	int32 namewidth = w - nameleft - st::dlgPaddingHor;
	QRect rectForName(nameleft, st::dlgPaddingVer + st::dlgNameTop, namewidth, st::msgNameFont->height);

	// draw chat icon
	if (history->peer->chat) {
		p.drawPixmap(QPoint(rectForName.left() + st::dlgChatImgLeft, rectForName.top() + st::dlgChatImgTop), App::sprite(), (act ? st::dlgActiveChatImg : st::dlgChatImg));
		rectForName.setLeft(rectForName.left() + st::dlgChatImgSkip);
	}

	// draw date
	QDateTime now(QDateTime::currentDateTime()), lastTime(_item->date);
	QDate nowDate(now.date()), lastDate(lastTime.date());
	QString dt;
	if (lastDate == nowDate) {
		dt = lastTime.toString(cTimeFormat());
	} else if (lastDate.year() == nowDate.year() && lastDate.weekNumber() == nowDate.weekNumber()) {
		dt = langDayOfWeek(lastDate);
	} else {
		dt = lastDate.toString(qsl("d.MM.yy"));
	}
	int32 dtWidth = st::dlgDateFont->m.width(dt);
	rectForName.setWidth(rectForName.width() - dtWidth - st::dlgDateSkip);
	p.setFont(st::dlgDateFont->f);
	p.setPen((act ? st::dlgActiveDateColor : st::dlgDateColor)->p);
	p.drawText(rectForName.left() + rectForName.width() + st::dlgDateSkip, rectForName.top() + st::msgNameFont->height - st::msgDateFont->descent, dt);

	// draw check
	if (_item->out() && _item->needCheck()) {
		const style::sprite *check;
		if (_item->id > 0) {
			if (_item->unread()) {
				check = act ? &st::dlgActiveCheckImg : &st::dlgCheckImg;
			} else {
				check = act ? &st::dlgActiveDblCheckImg : &st::dlgDblCheckImg;
			}
		} else {
			check = act ? &st::dlgActiveSendImg : &st::dlgSendImg;
		}
		rectForName.setWidth(rectForName.width() - check->pxWidth() - st::dlgCheckSkip);
		p.drawPixmap(QPoint(rectForName.left() + rectForName.width() + st::dlgCheckLeft, rectForName.top() + st::dlgCheckTop), App::sprite(), *check);
	}

	// draw unread
	int32 lastWidth = namewidth, unread = history->unreadCount;
	_item->drawInDialog(p, QRect(nameleft, st::dlgPaddingVer + st::dlgFont->height + st::dlgSep, lastWidth, st::dlgFont->height), act, _cacheFor, _cache);

	p.setPen((act ? st::dlgActiveColor : st::dlgNameColor)->p);
	history->nameText.drawElided(p, rectForName.left(), rectForName.top(), rectForName.width());
}

History::History(const PeerId &peerId) : width(0), height(0)
, msgCount(0)
, unreadCount(0)
, inboxReadTill(0)
, outboxReadTill(0)
, showFrom(0)
, unreadBar(0)
, peer(App::peer(peerId))
, oldLoaded(false)
, newLoaded(true)
, lastMsg(0)
, draftToId(0)
, lastWidth(0)
, lastScrollTop(History::ScrollMax)
, lastShowAtMsgId(ShowAtUnreadMsgId)
, mute(isNotifyMuted(peer->notify))
, lastKeyboardInited(false)
, lastKeyboardUsed(false)
, lastKeyboardId(0)
, lastKeyboardFrom(0)
, sendRequestId(0)
, textCachedFor(0)
, lastItemTextCache(st::dlgRichMinWidth)
, posInDialogs(0)
, typingText(st::dlgRichMinWidth)
{
	for (int32 i = 0; i < OverviewCount; ++i) {
		_overviewCount[i] = -1; // not loaded yet
	}
}

void History::clearLastKeyboard() {
	lastKeyboardInited = true;
	lastKeyboardId = 0;
	lastKeyboardFrom = 0;
}

void History::updateNameText() {
	nameText.setText(st::msgNameFont, peer->nameOrPhone.isEmpty() ? peer->name : peer->nameOrPhone, _textNameOptions);
}

bool History::updateTyping(uint64 ms, uint32 dots, bool force) {
	if (!ms) ms = getms(true);
	bool changed = force;
	for (TypingUsers::iterator i = typing.begin(), e = typing.end(); i != e;) {
		if (ms >= i.value()) {
			i = typing.erase(i);
			changed = true;
		} else {
			++i;
		}
	}
	for (SendActionUsers::iterator i = sendActions.begin(); i != sendActions.cend();) {
		if (ms >= i.value().until) {
			i = sendActions.erase(i);
			changed = true;
		} else {
			++i;
		}
	}
	if (changed) {
		QString newTypingStr;
		int32 cnt = typing.size();
		if (cnt > 2) {
			newTypingStr = lng_many_typing(lt_count, cnt);
		} else if (cnt > 1) {
			newTypingStr = lng_users_typing(lt_user, typing.begin().key()->firstName, lt_second_user, (typing.end() - 1).key()->firstName);
		} else if (cnt) {
			newTypingStr = peer->chat ? lng_user_typing(lt_user, typing.begin().key()->firstName) : lang(lng_typing);
		} else if (!sendActions.isEmpty()) {
			switch (sendActions.begin().value().type) {
			case SendActionRecordVideo: newTypingStr = peer->chat ? lng_user_action_record_video(lt_user, sendActions.begin().key()->firstName) : lang(lng_send_action_record_video); break;
			case SendActionUploadVideo: newTypingStr = peer->chat ? lng_user_action_upload_video(lt_user, sendActions.begin().key()->firstName) : lang(lng_send_action_upload_video); break;
			case SendActionRecordAudio: newTypingStr = peer->chat ? lng_user_action_record_audio(lt_user, sendActions.begin().key()->firstName) : lang(lng_send_action_record_audio); break;
			case SendActionUploadAudio: newTypingStr = peer->chat ? lng_user_action_upload_audio(lt_user, sendActions.begin().key()->firstName) : lang(lng_send_action_upload_audio); break;
			case SendActionUploadPhoto: newTypingStr = peer->chat ? lng_user_action_upload_photo(lt_user, sendActions.begin().key()->firstName) : lang(lng_send_action_upload_photo); break;
			case SendActionUploadFile: newTypingStr = peer->chat ? lng_user_action_upload_file(lt_user, sendActions.begin().key()->firstName) : lang(lng_send_action_upload_file); break;
			case SendActionChooseLocation: newTypingStr = peer->chat ? lng_user_action_geo_location(lt_user, sendActions.begin().key()->firstName) : lang(lng_send_action_geo_location); break;
			case SendActionChooseContact: newTypingStr = peer->chat ? lng_user_action_choose_contact(lt_user, sendActions.begin().key()->firstName) : lang(lng_send_action_choose_contact); break;
			}
		}
		if (!newTypingStr.isEmpty()) {
			newTypingStr += qsl("...");
		}
		if (typingStr != newTypingStr) {
			typingText.setText(st::dlgHistFont, (typingStr = newTypingStr), _textNameOptions);
		}
	}
	if (!typingStr.isEmpty()) {
		if (typingText.lastDots(dots % 4)) {
			changed = true;
		}
	}
	return changed;
}

void History::eraseFromOverview(MediaOverviewType type, MsgId msgId) {
	if (_overviewIds[type].isEmpty()) return;

	History::MediaOverviewIds::iterator i = _overviewIds[type].find(msgId);
	if (i == _overviewIds[type].cend()) return;

	_overviewIds[type].erase(i);
	for (History::MediaOverview::iterator i = _overview[type].begin(), e = _overview[type].end(); i != e; ++i) {
		if ((*i) == msgId) {
			_overview[type].erase(i);
			if (_overviewCount[type] > 0) {
				--_overviewCount[type];
				if (!_overviewCount[type]) {
					_overviewCount[type] = -1;
				}
			}
			break;
		}
	}
	if (App::wnd()) App::wnd()->mediaOverviewUpdated(peer, type);
}

bool DialogsList::del(const PeerId &peerId, DialogRow *replacedBy) {
	RowByPeer::iterator i = rowByPeer.find(peerId);
	if (i == rowByPeer.cend()) return false;

	DialogRow *row = i.value();
	emit App::main()->dialogRowReplaced(row, replacedBy);

	if (row == current) {
		current = row->next;
	}
	for (DialogRow *change = row->next; change != end; change = change->next) {
		change->pos--;
	}
	end->pos--;
	remove(row);
	delete row;
	--count;
	rowByPeer.erase(i);

	return true;
}

void DialogsIndexed::peerNameChanged(PeerData *peer, const PeerData::Names &oldNames, const PeerData::NameFirstChars &oldChars) {
	if (sortMode == DialogsSortByName) {
		DialogRow *mainRow = list.adjustByName(peer);
		if (!mainRow) return;

		History *history = mainRow->history;

		PeerData::NameFirstChars toRemove = oldChars, toAdd;
		for (PeerData::NameFirstChars::const_iterator i = peer->chars.cbegin(), e = peer->chars.cend(); i != e; ++i) {
			PeerData::NameFirstChars::iterator j = toRemove.find(*i);
			if (j == toRemove.cend()) {
				toAdd.insert(*i);
			} else {
				toRemove.erase(j);
				DialogsIndex::iterator k = index.find(*i);
				if (k != index.cend()) {
					k.value()->adjustByName(peer);
				}
			}
		}
		for (PeerData::NameFirstChars::const_iterator i = toRemove.cbegin(), e = toRemove.cend(); i != e; ++i) {
			DialogsIndex::iterator j = index.find(*i);
			if (j != index.cend()) {
				j.value()->del(peer->id, mainRow);
			}
		}
		if (!toAdd.isEmpty()) {
			for (PeerData::NameFirstChars::const_iterator i = toAdd.cbegin(), e = toAdd.cend(); i != e; ++i) {
				DialogsIndex::iterator j = index.find(*i);
				if (j == index.cend()) {
					j = index.insert(*i, new DialogsList(sortMode));
				}
				j.value()->addByName(history);
			}
		}
	} else {
		DialogsList::RowByPeer::const_iterator i = list.rowByPeer.find(peer->id);
		if (i == list.rowByPeer.cend()) return;

		DialogRow *mainRow = i.value();
		History *history = mainRow->history;

		PeerData::NameFirstChars toRemove = oldChars, toAdd;
		for (PeerData::NameFirstChars::const_iterator i = peer->chars.cbegin(), e = peer->chars.cend(); i != e; ++i) {
			PeerData::NameFirstChars::iterator j = toRemove.find(*i);
			if (j == toRemove.cend()) {
				toAdd.insert(*i);
			} else {
				toRemove.erase(j);
			}
		}
		for (PeerData::NameFirstChars::const_iterator i = toRemove.cbegin(), e = toRemove.cend(); i != e; ++i) {
			if (sortMode == DialogsSortByDate) history->dialogs.remove(*i);
			DialogsIndex::iterator j = index.find(*i);
			if (j != index.cend()) {
				j.value()->del(peer->id, mainRow);
			}
		}
		for (PeerData::NameFirstChars::const_iterator i = toAdd.cbegin(), e = toAdd.cend(); i != e; ++i) {
			DialogsIndex::iterator j = index.find(*i);
			if (j == index.cend()) {
				j = index.insert(*i, new DialogsList(sortMode));
			}
			if (sortMode == DialogsSortByDate) {
				history->dialogs.insert(*i, j.value()->addByPos(history));
			} else {
				j.value()->addToEnd(history);
			}
		}
	}
}

void DialogsIndexed::clear() {
	for (DialogsIndex::iterator i = index.begin(), e = index.end(); i != e; ++i) {
		delete i.value();
	}
	index.clear();
	list.clear();
}

void Histories::clear() {
	App::historyClearMsgs();
	for (Parent::const_iterator i = cbegin(), e = cend(); i != e; ++i) {
		delete i.value();
	}
	App::historyClearItems();
	typing.clear();
	Parent::clear();
}

void Histories::regSendAction(History *history, UserData *user, const MTPSendMessageAction &action) {
	if (action.type() == mtpc_sendMessageCancelAction) {
		history->unregTyping(user);
		return;
	}

	uint64 ms = getms(true);
	switch (action.type()) {
	case mtpc_sendMessageTypingAction: history->typing[user] = ms + 6000; break;
	case mtpc_sendMessageRecordVideoAction: history->sendActions.insert(user, SendAction(SendActionRecordVideo, ms + 6000)); break;
	case mtpc_sendMessageUploadVideoAction: history->sendActions.insert(user, SendAction(SendActionUploadVideo, ms + 6000, action.c_sendMessageUploadVideoAction().vprogress.v)); break;
	case mtpc_sendMessageRecordAudioAction: history->sendActions.insert(user, SendAction(SendActionRecordAudio, ms + 6000)); break;
	case mtpc_sendMessageUploadAudioAction: history->sendActions.insert(user, SendAction(SendActionUploadAudio, ms + 6000, action.c_sendMessageUploadAudioAction().vprogress.v)); break;
	case mtpc_sendMessageUploadPhotoAction: history->sendActions.insert(user, SendAction(SendActionUploadPhoto, ms + 6000, action.c_sendMessageUploadPhotoAction().vprogress.v)); break;
	case mtpc_sendMessageUploadDocumentAction: history->sendActions.insert(user, SendAction(SendActionUploadFile, ms + 6000, action.c_sendMessageUploadDocumentAction().vprogress.v)); break;
	case mtpc_sendMessageGeoLocationAction: history->sendActions.insert(user, SendAction(SendActionChooseLocation, ms + 6000)); break;
	case mtpc_sendMessageChooseContactAction: history->sendActions.insert(user, SendAction(SendActionChooseContact, ms + 6000)); break;
	default: return;
	}

	user->madeAction();

	TypingHistories::const_iterator i = typing.find(history);
	if (i == typing.cend()) {
		typing.insert(history, ms);
		history->typingFrame = 0;
	}

	history->updateTyping(ms, history->typingFrame, true);
	anim::start(this);
}

bool Histories::animStep(float64) {
	uint64 ms = getms(true);
	for (TypingHistories::iterator i = typing.begin(), e = typing.end(); i != e;) {
		uint32 typingFrame = (ms - i.value()) / 150;
		if (i.key()->updateTyping(ms, typingFrame)) {
			App::main()->dlgUpdated(i.key());
			App::main()->topBar()->update();
		}
		if (i.key()->typing.isEmpty() && i.key()->sendActions.isEmpty()) {
			i = typing.erase(i);
		} else {
			++i;
		}
	}
	return !typing.isEmpty();
}

Histories::Parent::iterator Histories::erase(Histories::Parent::iterator i) {
	typing.remove(i.value());
	delete i.value();
	return Parent::erase(i);
}

void Histories::remove(const PeerId &peer) {
	iterator i = find(peer);
	if (i != cend()) {
		erase(i);
	}
	
}

HistoryItem *Histories::addToBack(const MTPmessage &msg, int msgState) {
	PeerId from_id = 0, to_id = 0;
	switch (msg.type()) {
	case mtpc_message:
		from_id = App::peerFromUser(msg.c_message().vfrom_id);
		to_id = App::peerFromMTP(msg.c_message().vto_id);
	break;
	case mtpc_messageService:
		from_id = App::peerFromUser(msg.c_messageService().vfrom_id);
		to_id = App::peerFromMTP(msg.c_messageService().vto_id);
	break;
	}
	PeerId peer = (to_id == App::peerFromUser(MTP::authedId())) ? from_id : to_id;

	if (!peer) return 0;

	iterator h = find(peer);
	if (h == end()) {
		h = insert(peer, new History(peer));
	}
	if (msgState < 0) {
		return h.value()->addToHistory(msg);
	}
	if (!h.value()->loadedAtBottom()) {
		HistoryItem *item = h.value()->addToHistory(msg);
		if (item) {
			h.value()->setLastMessage(item);
			if (msgState > 0) {
				h.value()->newItemAdded(item);
			}
		}
		return item;
	}
	return h.value()->addToBack(msg, msgState > 0);
}

HistoryItem *History::createItem(HistoryBlock *block, const MTPmessage &msg, bool newMsg, bool returnExisting) {
	HistoryItem *result = 0;

	MsgId msgId = 0;
	switch (msg.type()) {
	case mtpc_messageEmpty: msgId = msg.c_messageEmpty().vid.v; break;
	case mtpc_message: msgId = msg.c_message().vid.v; break;
	case mtpc_messageService: msgId = msg.c_messageService().vid.v; break;
	}

	HistoryItem *existing = App::histItemById(msgId);
	if (existing) {
		bool regged = false;
		if (existing->detached() && block) {
			existing->attach(block);
			regged = true;
		}

		const MTPMessageMedia *media = 0;
		switch (msg.type()) {
		case mtpc_message:
			media = msg.c_message().has_media() ? (&msg.c_message().vmedia) : 0;
		break;
		}
		if (media) {
			existing->updateMedia(*media);
		}
		return (returnExisting || regged) ? existing : 0;
	}

	switch (msg.type()) {
	case mtpc_messageEmpty:
		result = new HistoryServiceMsg(this, block, msg.c_messageEmpty().vid.v, date(), lang(lng_message_empty));
	break;

	case mtpc_message: {
		const MTPDmessage m(msg.c_message());
		int badMedia = 0; // 1 - unsupported, 2 - empty
		if (m.has_media()) switch (m.vmedia.type()) {
		case mtpc_messageMediaEmpty:
		case mtpc_messageMediaContact: break;
		case mtpc_messageMediaGeo:
			switch (m.vmedia.c_messageMediaGeo().vgeo.type()) {
			case mtpc_geoPoint: break;
			case mtpc_geoPointEmpty: badMedia = 2; break;
			default: badMedia = 1; break;
			}
			break;
		case mtpc_messageMediaVenue:
			switch (m.vmedia.c_messageMediaVenue().vgeo.type()) {
			case mtpc_geoPoint: break;
			case mtpc_geoPointEmpty: badMedia = 2; break;
			default: badMedia = 1; break;
			}
			break;
		case mtpc_messageMediaPhoto:
			switch (m.vmedia.c_messageMediaPhoto().vphoto.type()) {
			case mtpc_photo: break;
			case mtpc_photoEmpty: badMedia = 2; break;
			default: badMedia = 1; break;
			}
			break;
		case mtpc_messageMediaVideo:
			switch (m.vmedia.c_messageMediaVideo().vvideo.type()) {
			case mtpc_video: break;
			case mtpc_videoEmpty: badMedia = 2; break;
			default: badMedia = 1; break;
			}
			break;
		case mtpc_messageMediaAudio:
			switch (m.vmedia.c_messageMediaAudio().vaudio.type()) {
			case mtpc_audio: break;
			case mtpc_audioEmpty: badMedia = 2; break;
			default: badMedia = 1; break;
			}
			break;
		case mtpc_messageMediaDocument:
			switch (m.vmedia.c_messageMediaDocument().vdocument.type()) {
			case mtpc_document: break;
			case mtpc_documentEmpty: badMedia = 2; break;
			default: badMedia = 1; break;
			}
			break;
		case mtpc_messageMediaWebPage:
			switch (m.vmedia.c_messageMediaWebPage().vwebpage.type()) {
			case mtpc_webPage:
			case mtpc_webPageEmpty:
			case mtpc_webPagePending: break;
			default: badMedia = 1; break;
			}
			break;
		case mtpc_messageMediaUnsupported:
		default: badMedia = 1; break;
		}
		if (badMedia) {
			result = new HistoryServiceMsg(this, block, m.vid.v, date(m.vdate), lang((badMedia == 2) ? lng_message_empty : lng_media_unsupported), m.vflags.v, 0, m.vfrom_id.v);
		} else {
			if ((m.has_fwd_date() && m.vfwd_date.v > 0) || (m.has_fwd_from_id() && m.vfwd_from_id.v != 0)) {
				result = new HistoryForwarded(this, block, m);
			} else if (m.has_reply_to_msg_id() && m.vreply_to_msg_id.v > 0) {
				result = new HistoryReply(this, block, m);
			} else {
				result = new HistoryMessage(this, block, m);
			}
			if (m.has_reply_markup()) {
				App::feedReplyMarkup(msgId, m.vreply_markup);
			}
		}
	} break;

	case mtpc_messageService: {
		const MTPDmessageService &d(msg.c_messageService());
		result = new HistoryServiceMsg(this, block, d);

		if (newMsg) {
			const MTPmessageAction &action(d.vaction);
			switch (d.vaction.type()) {
			case mtpc_messageActionChatAddUser: {
				const MTPDmessageActionChatAddUser &d(action.c_messageActionChatAddUser());
				// App::user(App::peerFromUser(d.vuser_id)); added
			} break;

			case mtpc_messageActionChatJoinedByLink: {
				const MTPDmessageActionChatJoinedByLink &d(action.c_messageActionChatJoinedByLink());
				// App::user(App::peerFromUser(d.vuser_id)); added
			} break;

			case mtpc_messageActionChatDeletePhoto: {
				ChatData *chat = peer->asChat();
				if (chat) chat->setPhoto(MTP_chatPhotoEmpty());
			} break;

			case mtpc_messageActionChatDeleteUser: {
				const MTPDmessageActionChatDeleteUser &d(action.c_messageActionChatDeleteUser());
				if (lastKeyboardFrom == App::peerFromUser(d.vuser_id)) {
					clearLastKeyboard();
				}
				// App::peer(App::peerFromUser(d.vuser_id)); left
			} break;

			case mtpc_messageActionChatEditPhoto: {
				const MTPDmessageActionChatEditPhoto &d(action.c_messageActionChatEditPhoto());
				if (d.vphoto.type() == mtpc_photo) {
					const QVector<MTPPhotoSize> &sizes(d.vphoto.c_photo().vsizes.c_vector().v);
					if (!sizes.isEmpty()) {
						ChatData *chat = peer->asChat();
						if (chat) {
							PhotoData *photo = App::feedPhoto(d.vphoto.c_photo());
							if (photo) photo->chat = chat;
							const MTPPhotoSize &smallSize(sizes.front()), &bigSize(sizes.back());
							const MTPFileLocation *smallLoc = 0, *bigLoc = 0;
							switch (smallSize.type()) {
							case mtpc_photoSize: smallLoc = &smallSize.c_photoSize().vlocation; break;
							case mtpc_photoCachedSize: smallLoc = &smallSize.c_photoCachedSize().vlocation; break;
							}
							switch (bigSize.type()) {
							case mtpc_photoSize: bigLoc = &bigSize.c_photoSize().vlocation; break;
							case mtpc_photoCachedSize: bigLoc = &bigSize.c_photoCachedSize().vlocation; break;
							}
							if (smallLoc && bigLoc) {
								chat->setPhoto(MTP_chatPhoto(*smallLoc, *bigLoc), photo ? photo->id : 0);
								chat->photo->load();
							}
						}
					}
				}
			} break;

			case mtpc_messageActionChatEditTitle: {
				const MTPDmessageActionChatEditTitle &d(action.c_messageActionChatEditTitle());
				ChatData *chat = peer->asChat();
				if (chat) chat->updateName(qs(d.vtitle), QString(), QString());
			} break;
			}
		}
	} break;
	}

	return regItem(result, returnExisting);
}

HistoryItem *History::createItemForwarded(HistoryBlock *block, MsgId id, HistoryMessage *msg) {
	HistoryItem *result = 0;

	result = new HistoryForwarded(this, block, id, msg);

	return regItem(result);
}

HistoryItem *History::createItemDocument(HistoryBlock *block, MsgId id, int32 flags, MsgId replyTo, QDateTime date, int32 from, DocumentData *doc) {
	HistoryItem *result = 0;

	if (flags & MTPDmessage::flag_reply_to_msg_id && replyTo > 0) {
		result = new HistoryReply(this, block, id, flags, replyTo, date, from, doc);
	} else {
		result = new HistoryMessage(this, block, id, flags, date, from, doc);
	}

	return regItem(result);
}

HistoryItem *History::addToBackService(MsgId msgId, QDateTime date, const QString &text, int32 flags, HistoryMedia *media, bool newMsg) {
	HistoryBlock *to = 0;
	bool newBlock = isEmpty();
	if (newBlock) {
		to = new HistoryBlock(this);
	} else {
		to = back();
	}

	return doAddToBack(to, newBlock, regItem(new HistoryServiceMsg(this, to, msgId, date, text, flags, media)), newMsg);
}

HistoryItem *History::addToBack(const MTPmessage &msg, bool newMsg) {
	HistoryBlock *to = 0;
	bool newBlock = isEmpty();
	if (newBlock) {
		to = new HistoryBlock(this);
	} else {
		to = back();
	}
	return doAddToBack(to, newBlock, createItem(to, msg, newMsg), newMsg);
}

HistoryItem *History::addToHistory(const MTPmessage &msg) {
	return createItem(0, msg, false, true);
}

HistoryItem *History::addToBackForwarded(MsgId id, HistoryMessage *item) {
	HistoryBlock *to = 0;
	bool newBlock = isEmpty();
	if (newBlock) {
		to = new HistoryBlock(this);
	} else {
		to = back();
	}
	return doAddToBack(to, newBlock, createItemForwarded(to, id, item), true);
}

HistoryItem *History::addToBackDocument(MsgId id, int32 flags, MsgId replyTo, QDateTime date, int32 from, DocumentData *doc) {
	HistoryBlock *to = 0;
	bool newBlock = isEmpty();
	if (newBlock) {
		to = new HistoryBlock(this);
	} else {
		to = back();
	}
	return doAddToBack(to, newBlock, createItemDocument(to, id, flags, replyTo, date, from, doc), true);
}

void History::createInitialDateBlock(const QDateTime &date) {
	HistoryBlock *dateBlock = new HistoryBlock(this); // date block
	HistoryItem *dayItem = createDayServiceMsg(this, dateBlock, date);
	dateBlock->push_back(dayItem);
	if (width) {
		int32 dh = dayItem->resize(width);
		dateBlock->height = dh;
		height += dh;
		for (int32 i = 0, l = size(); i < l; ++i) {
			(*this)[i]->y += dh;
		}
	}
	push_front(dateBlock); // date block
}

void History::addToOverview(HistoryItem *item, MediaOverviewType type) {
	if (_overviewIds[type].constFind(item->id) == _overviewIds[type].cend()) {
		_overview[type].push_back(item->id);
		_overviewIds[type].insert(item->id, NullType());
		if (_overviewCount[type] > 0) ++_overviewCount[type];
		if (App::wnd()) App::wnd()->mediaOverviewUpdated(peer, type);
	}
}

bool History::addToOverviewFront(HistoryItem *item, MediaOverviewType type) {
	if (_overviewIds[type].constFind(item->id) == _overviewIds[type].cend()) {
		_overview[type].push_front(item->id);
		_overviewIds[type].insert(item->id, NullType());
		return true;
	}
	return false;
}

HistoryItem *History::doAddToBack(HistoryBlock *to, bool newBlock, HistoryItem *adding, bool newMsg) {
	if (!adding) {
		if (newBlock) delete to;
		return adding;
	}

	if (newBlock) {
		createInitialDateBlock(adding->date);

		to->y = height;
		push_back(to);
	} else if (to->back()->date.date() != adding->date.date()) {
		HistoryItem *dayItem = createDayServiceMsg(this, to, adding->date);
		to->push_back(dayItem);
		dayItem->y = to->height;
		if (width) {
			int32 dh = dayItem->resize(width);
			to->height += dh;
			height += dh;
		}
	}
	to->push_back(adding);
	setLastMessage(adding);

	adding->y = to->height;
	if (width) {
		int32 dh = adding->resize(width);
		to->height += dh;
		height += dh;
	}
	setMsgCount(msgCount + 1);
	if (newMsg) {
		newItemAdded(adding);
	}

	HistoryMedia *media = adding->getMedia(true);
	if (media) {
		HistoryMediaType mt = media->type();
		MediaOverviewType t = mediaToOverviewType(mt);
		if (t != OverviewCount) {
			addToOverview(adding, t);
			if (mt == MediaTypeDocument && static_cast<HistoryDocument*>(media)->document()->song()) {
				addToOverview(adding, OverviewAudioDocuments);
			}
		}
	}
	if (adding->hasTextLinks()) {
		addToOverview(adding, OverviewLinks);
	}
	if (adding->from()->id) {
		if (peer->chat) {
			QList<UserData*> *lastAuthors = &(peer->asChat()->lastAuthors);
			int prev = lastAuthors->indexOf(adding->from());
			if (prev > 0) {
				lastAuthors->removeAt(prev);
			}
			if (prev) {
				lastAuthors->push_front(adding->from());
			}
		}
		if (adding->hasReplyMarkup()) {
			int32 markupFlags = App::replyMarkup(adding->id).flags;
			if (!(markupFlags & MTPDreplyKeyboardMarkup_flag_personal) || adding->notifyByFrom()) {
				if (peer->chat) {
					peer->asChat()->markupSenders.insert(adding->from(), true);
				}
				if (markupFlags & MTPDreplyKeyboardMarkup_flag_ZERO) { // zero markup means replyKeyboardHide
					if (lastKeyboardFrom == adding->from()->id || (!lastKeyboardInited && !peer->chat && !adding->out())) {
						clearLastKeyboard();
					}
				} else if (peer->chat && (peer->asChat()->count < 1 || !peer->asChat()->participants.isEmpty()) && !peer->asChat()->participants.contains(adding->from())) {
					clearLastKeyboard();
				} else {
					lastKeyboardInited = true;
					lastKeyboardId = adding->id;
					lastKeyboardFrom = adding->from()->id;
					lastKeyboardUsed = false;
				}
			}
		}
	}
	return adding;
}

void History::unregTyping(UserData *from) {
	bool update = false;
	uint64 updateAtMs = 0;
	TypingUsers::iterator i = typing.find(from);
	if (i != typing.end()) {
		updateAtMs = getms(true);
		i.value() = updateAtMs;
		update = true;
	}
	SendActionUsers::iterator j = sendActions.find(from);
	if (j != sendActions.end()) {
		if (!updateAtMs) updateAtMs = getms(true);
		j.value().until = updateAtMs;
		update = true;
	}
	if (updateAtMs) {
		updateTyping(updateAtMs, 0, true);
		App::main()->topBar()->update();
	}
}

void History::newItemAdded(HistoryItem *item) {
	App::checkImageCacheSize();
	if (item->from()) {
		unregTyping(item->from());
		item->from()->madeAction();
	}
	if (item->out()) {
		if (unreadBar) unreadBar->destroy();
	} else if (item->unread()) {
		notifies.push_back(item);
		App::main()->newUnreadMsg(this, item);
	}
	if (dialogs.isEmpty()) {
		App::main()->createDialogAtTop(this, unreadCount);
	} else {
		emit App::main()->dialogToTop(dialogs);
	}
}

void History::addToFront(const QVector<MTPMessage> &slice) {
	if (slice.isEmpty()) {
		oldLoaded = true;
		return;
	}

	int32 addToH = 0, skip = 0;
	if (!isEmpty()) {
		if (width) addToH = -front()->height;
		pop_front(); // remove date block
	}
	HistoryItem *till = isEmpty() ? 0 : front()->front(), *prev = 0;

	HistoryBlock *block = new HistoryBlock(this);
	block->reserve(slice.size());
	int32 wasMsgCount = msgCount;
	for (QVector<MTPmessage>::const_iterator i = slice.cend() - 1, e = slice.cbegin(); ; --i) {
		HistoryItem *adding = createItem(block, *i, false);
		if (adding) {
			if (prev && prev->date.date() != adding->date.date()) {
				HistoryItem *dayItem = createDayServiceMsg(this, block, adding->date);
				block->push_back(dayItem);
				if (width) {
					dayItem->y = block->height;
					block->height += dayItem->resize(width);
				}
			}
			block->push_back(adding);
			if (width) {
				adding->y = block->height;
				block->height += adding->resize(width);
			}
			setMsgCount(msgCount + 1);
			prev = adding;
		}
		if (i == e) break;
	}
	if (till && prev && prev->date.date() != till->date.date()) {
		HistoryItem *dayItem = createDayServiceMsg(this, block, till->date);
		block->push_back(dayItem);
		if (width) {
			dayItem->y = block->height;
			block->height += dayItem->resize(width);
		}
	}
	if (block->size()) {
		if (loadedAtBottom() && wasMsgCount < unreadCount && msgCount >= unreadCount) {
			for (int32 i = block->size(); i > 0; --i) {
				if ((*block)[i - 1]->itemType() == HistoryItem::MsgType) {
					++wasMsgCount;
					if (wasMsgCount == unreadCount) {
						showFrom = (*block)[i - 1];
						break;
					}
				}
			}
		}
		push_front(block);
		if (width) {
			addToH += block->height;
			++skip;
		}

		if (loadedAtBottom()) { // add photos to overview and authors to lastAuthors
			int32 mask = 0;
			QList<UserData*> *lastAuthors = peer->chat ? &(peer->asChat()->lastAuthors) : 0;
			for (int32 i = block->size(); i > 0; --i) {
				HistoryItem *item = (*block)[i - 1];
				HistoryMedia *media = item->getMedia(true);
				if (media) {
					HistoryMediaType mt = media->type();
					MediaOverviewType t = mediaToOverviewType(mt);
					if (t != OverviewCount) {
						if (addToOverviewFront(item, t)) mask |= (1 << t);
						if (mt == MediaTypeDocument && static_cast<HistoryDocument*>(media)->document()->song()) {
							if (addToOverviewFront(item, OverviewAudioDocuments)) mask |= (1 << OverviewAudioDocuments);
						}
					}
				}
				if (item->hasTextLinks()) {
					if (addToOverviewFront(item, OverviewLinks)) mask |= (1 << OverviewLinks);
				}
				if (item->from()->id) {
					if (lastAuthors) { // chats
						if (!lastAuthors->contains(item->from())) {
							lastAuthors->push_back(item->from());
						}
						if (!lastKeyboardInited && item->hasReplyMarkup() && !item->out()) { // chats with bots
							int32 markupFlags = App::replyMarkup(item->id).flags;
							if (!(markupFlags & MTPDreplyKeyboardMarkup_flag_personal) || item->notifyByFrom()) {
								bool wasKeyboardHide = peer->asChat()->markupSenders.contains(item->from());
								if (!wasKeyboardHide) {
									peer->asChat()->markupSenders.insert(item->from(), true);
								}
								if (!(markupFlags & MTPDreplyKeyboardMarkup_flag_ZERO)) {
									if (!lastKeyboardInited) {
										if (wasKeyboardHide || ((peer->asChat()->count < 1 || !peer->asChat()->participants.isEmpty()) && !peer->asChat()->participants.contains(item->from()))) {
											clearLastKeyboard();
										} else {
											lastKeyboardInited = true;
											lastKeyboardId = item->id;
											lastKeyboardFrom = item->from()->id;
											lastKeyboardUsed = false;
										}
									}
								}
							}
						}
					} else if (!lastKeyboardInited && item->hasReplyMarkup() && !item->out()) { // conversations with bots
						int32 markupFlags = App::replyMarkup(item->id).flags;
						if (!(markupFlags & MTPDreplyKeyboardMarkup_flag_personal) || item->notifyByFrom()) {
							if (markupFlags & MTPDreplyKeyboardMarkup_flag_ZERO) {
								clearLastKeyboard();
							} else {
								lastKeyboardInited = true;
								lastKeyboardId = item->id;
								lastKeyboardFrom = item->from()->id;
								lastKeyboardUsed = false;
							}
						}
					}
				}
			}
			for (int32 t = 0; t < OverviewCount; ++t) {
				if ((mask & (1 << t)) && App::wnd()) App::wnd()->mediaOverviewUpdated(peer, MediaOverviewType(t));
			}
		}
	} else {
		delete block;
	}
	if (!isEmpty()) {
		HistoryBlock *dateBlock = new HistoryBlock(this);
		HistoryItem *dayItem = createDayServiceMsg(this, dateBlock, front()->front()->date);
		dateBlock->push_back(dayItem);
		if (width) {
			int32 dh = dayItem->resize(width);
			dateBlock->height = dh;
			if (skip) {
				front()->y += dh;
			}
			addToH += dh;
			++skip;
		}
		push_front(dateBlock); // date block
	}
	if (width && addToH) {
		for (iterator i = begin(), e = end(); i != e; ++i) {
			if (skip) {
				--skip;
			} else {
				(*i)->y += addToH;
			}
		}
		height += addToH;
	}
}

void History::addToBack(const QVector<MTPMessage> &slice) {
	if (slice.isEmpty()) {
		newLoaded = true;
		return;
	}

	bool wasEmpty = isEmpty();

	HistoryItem *prev = isEmpty() ? 0 : back()->back();

	HistoryBlock *block = new HistoryBlock(this);
	block->reserve(slice.size());
	int32 wasMsgCount = msgCount;
	for (QVector<MTPmessage>::const_iterator i = slice.cend(), e = slice.cbegin(); i != e;) {
		--i;
		HistoryItem *adding = createItem(block, *i, false);
		if (adding) {
			if (prev && prev->date.date() != adding->date.date()) {
				HistoryItem *dayItem = createDayServiceMsg(this, block, adding->date);
				prev->block()->push_back(dayItem);
				dayItem->y = prev->block()->height;
				prev->block()->height += dayItem->resize(width);
				if (prev->block() != block) {
					height += dayItem->height();
				}
			}
			block->push_back(adding);
			adding->y = block->height;
			block->height += adding->resize(width);
			setMsgCount(msgCount + 1);
			prev = adding;
		}
		if (i == e) break;
	}
	bool wasLoadedAtBottom = loadedAtBottom();
	if (block->size()) {
		block->y = height;
		push_back(block);
		height += block->height;
	} else {
		newLoaded = true;
		fixLastMessage(true);
		delete block;
	}
	if (!wasLoadedAtBottom && loadedAtBottom()) { // add all loaded photos to overview
		int32 mask = 0;
		for (int32 i = 0; i < OverviewCount; ++i) {
			if (_overviewCount[i] == 0) continue; // all loaded
			if (!_overview[i].isEmpty() || !_overviewIds[i].isEmpty()) {
				_overview[i].clear();
				_overviewIds[i].clear();
				mask |= (1 << i);
			}
		}
		for (int32 i = 0; i < size(); ++i) {
			HistoryBlock *b = (*this)[i];
			for (int32 j = 0; j < b->size(); ++j) {
				HistoryItem *item = (*b)[j];
				HistoryMedia *media = item->getMedia(true);
				if (media) {
					HistoryMediaType mt = media->type();
					MediaOverviewType t = mediaToOverviewType(mt);
					if (t != OverviewCount) {
						if (_overviewCount[t] != 0) {
							_overview[t].push_back(item->id);
							_overviewIds[t].insert(item->id, NullType());
							mask |= (1 << t);
						}
						if (mt == MediaTypeDocument && static_cast<HistoryDocument*>(media)->document()->song()) {
							t = OverviewAudioDocuments;
							if (_overviewCount[t] != 0) {
								_overview[t].push_back(item->id);
								_overviewIds[t].insert(item->id, NullType());
								mask |= (1 << t);
							}
						}
					}
				}
				if (item->hasTextLinks()) {
					MediaOverviewType t = OverviewLinks;
					if (_overviewCount[t] != 0) {
						_overview[t].push_back(item->id);
						_overviewIds[t].insert(item->id, NullType());
						mask |= (1 << t);
					}
				}
			}
		}
		for (int32 t = 0; t < OverviewCount; ++t) {
			if ((mask & (1 << t)) && App::wnd()) App::wnd()->mediaOverviewUpdated(peer, MediaOverviewType(t));
		}
	}
	if (wasEmpty && !isEmpty()) {
		HistoryBlock *dateBlock = new HistoryBlock(this);
		HistoryItem *dayItem = createDayServiceMsg(this, dateBlock, front()->front()->date);
		dateBlock->push_back(dayItem);
		int32 dh = dayItem->resize(width);
		dateBlock->height = dh;
		for (iterator i = begin(), e = end(); i != e; ++i) {
			(*i)->y += dh;
		}
		push_front(dateBlock); // date block
		height += dh;
	}
}

void History::inboxRead(int32 upTo) {
	if (unreadCount) {
		if (upTo && loadedAtBottom()) App::main()->historyToDown(this);
		setUnreadCount(0);
	}
	if (!isEmpty()) {
		int32 till = upTo ? upTo : back()->back()->id;
		if (inboxReadTill < till) inboxReadTill = till;
	}
	if (!dialogs.isEmpty()) {
		if (App::main()) App::main()->dlgUpdated(dialogs[0]);
	}
	showFrom = 0;
	App::wnd()->notifyClear(this);
	clearNotifications();
}

void History::inboxRead(HistoryItem *wasRead) {
	return inboxRead(wasRead ? wasRead->id : 0);
}

void History::outboxRead(int32 upTo) {
	if (!isEmpty()) {
		int32 till = upTo ? upTo : back()->back()->id;
		if (outboxReadTill < till) outboxReadTill = till;
	}
}

void History::outboxRead(HistoryItem *wasRead) {
	return outboxRead(wasRead ? wasRead->id : 0);
}

void History::setUnreadCount(int32 newUnreadCount, bool psUpdate) {
	if (unreadCount != newUnreadCount) {
		if (newUnreadCount == 1 && loadedAtBottom()) {
			showFrom = isEmpty() ? 0 : back()->back();
		} else if (!newUnreadCount) {
			showFrom = 0;
		}
		App::histories().unreadFull += newUnreadCount - unreadCount;
		if (mute) App::histories().unreadMuted += newUnreadCount - unreadCount;
		unreadCount = newUnreadCount;
		if (psUpdate && (!mute || cIncludeMuted())) App::wnd()->updateCounter();
		if (unreadBar) unreadBar->setCount(unreadCount);
	}
}

void History::setMsgCount(int32 newMsgCount) {
	if (msgCount != newMsgCount) {
		msgCount = newMsgCount;
	}
}

 void History::setMute(bool newMute) {
	if (mute != newMute) {
		App::histories().unreadMuted += newMute ? unreadCount : (-unreadCount);
		mute = newMute;
		if (App::wnd()) App::wnd()->updateCounter();
		if (App::main()) App::main()->dlgUpdated(this);
	}
}

void History::getNextShowFrom(HistoryBlock *block, int32 i) {
	if (i >= 0) {
		int32 l = block->size();
		for (++i; i < l; ++i) {
			if ((*block)[i]->itemType() == HistoryItem::MsgType) {
				showFrom = (*block)[i];
				return;
			}
		}
	}

	int32 j = indexOf(block), s = size();
	if (j >= 0) {
		for (++j; j < s; ++j) {
			block = (*this)[j];
			for (int32 i = 0, l = block->size(); i < l; ++i) {
				if ((*block)[i]->itemType() == HistoryItem::MsgType) {
					showFrom = (*block)[i];
					return;
				}
			}
		}
	}
	showFrom = 0;
}

void History::addUnreadBar() {
	if (unreadBar || !showFrom || showFrom->detached() || !unreadCount) return;

	HistoryBlock *block = showFrom->block();
	int32 i = block->indexOf(showFrom);
	int32 j = indexOf(block);
	if (i < 0 || j < 0) return;

	HistoryUnreadBar *bar = new HistoryUnreadBar(this, block, unreadCount, showFrom->date);
	block->insert(i, bar);
	unreadBar = bar;

	unreadBar->y = showFrom->y;

	int32 dh = unreadBar->resize(width), l = block->size();
	for (++i; i < l; ++i) {
		(*block)[i]->y += dh;
	}
	block->height += dh;
	for (++j, l = size(); j < l; ++j) {
		(*this)[j]->y += dh;
	}
	height += dh;
}

void History::clearNotifications() {
	notifies.clear();
}

bool History::loadedAtBottom() const {
	return newLoaded;
}

bool History::loadedAtTop() const {
	return oldLoaded;
}

bool History::isReadyFor(MsgId msgId, bool check) const {
	if (msgId == ShowAtTheEndMsgId) {
		return loadedAtBottom();
	} else if (msgId == ShowAtUnreadMsgId) {
		return check ? (loadedAtBottom() && (msgCount >= unreadCount)) : !isEmpty();
	} else if (check) {
		HistoryItem *item = App::histItemById(msgId);
		return item && item->history() == this && !item->detached();
	}
	return !isEmpty();
}

void History::getReadyFor(MsgId msgId) {
	if (!isReadyFor(msgId, true)) {
		clear(true);
		newLoaded = (msgId == ShowAtTheEndMsgId) || (lastMsg && !lastMsg->detached());
		oldLoaded = false;
		lastWidth = 0;
		lastShowAtMsgId = msgId;
	}
}

void History::setLastMessage(HistoryItem *msg) {
	if (msg) {
		if (!lastMsg) Local::removeSavedPeer(peer);
		lastMsg = msg;
		lastMsgDate = msg->date;
	} else {
		lastMsg = 0;
	}
}

void History::fixLastMessage(bool wasAtBottom) {
	if (wasAtBottom && isEmpty()) {
		wasAtBottom = false;
	}
	if (wasAtBottom) {
		setLastMessage(back()->back());
	} else {
		setLastMessage(0);
		if (App::main()) {
			App::main()->checkPeerHistory(peer);
		}
	}
}

MsgId History::minMsgId() const {
	for (const_iterator i = cbegin(), e = cend(); i != e; ++i) {
		for (HistoryBlock::const_iterator j = (*i)->cbegin(), en = (*i)->cend(); j != en; ++j) {
			if ((*j)->id > 0) {
				return (*j)->id;
			}
		}
	}
	return 0;
}

MsgId History::maxMsgId() const {
	for (const_iterator i = cend(), e = cbegin(); i != e;) {
		--i;
		for (HistoryBlock::const_iterator j = (*i)->cend(), en = (*i)->cbegin(); j != en;) {
			--j;
			if ((*j)->id > 0) {
				return (*j)->id;
			}
		}
	}
	return 0;
}

int32 History::geomResize(int32 newWidth, int32 *ytransform, bool dontRecountText) {
	if (width != newWidth || dontRecountText) {
		int32 y = 0;
		for (iterator i = begin(), e = end(); i != e; ++i) {
			HistoryBlock *block = *i;
			bool updTransform = ytransform && (*ytransform >= block->y) && (*ytransform < block->y + block->height);
			if (updTransform) *ytransform -= block->y;
			if (block->y != y) {
				block->y = y;
			}
			y += block->geomResize(newWidth, ytransform, dontRecountText);
			if (updTransform) {
				*ytransform += block->y;
				ytransform = 0;
			}
		}
		width = newWidth;
		height = y;
	}
	return height;
}

void History::clear(bool leaveItems) {
	if (unreadBar) {
		unreadBar->destroy();
	}
	if (showFrom) {
		showFrom = 0;
	}
	if (!leaveItems) {
		setLastMessage(0);
	}
	for (int32 i = 0; i < OverviewCount; ++i) {
		if (!_overview[i].isEmpty() || !_overviewIds[i].isEmpty()) {
			if (leaveItems) {
				if (_overviewCount[i] == 0) _overviewCount[i] = _overview[i].size();
			} else {
				_overviewCount[i] = -1; // not loaded yet
			}
			_overview[i].clear();
			_overviewIds[i].clear();
			if (App::wnd() && !App::quiting()) App::wnd()->mediaOverviewUpdated(peer, MediaOverviewType(i));
		}
	}
	for (Parent::const_iterator i = cbegin(), e = cend(); i != e; ++i) {
		if (leaveItems) {
			(*i)->clear(true);
		}
		delete *i;
	}
	Parent::clear();
	setMsgCount(0);
	if (leaveItems) {
		lastKeyboardInited = false;
	} else {
		setUnreadCount(0);
	}
	height = 0;
	oldLoaded = false;
	if (peer->chat) {
		peer->asChat()->lastAuthors.clear();
		peer->asChat()->markupSenders.clear();
	}
	if (leaveItems && App::main()) App::main()->historyCleared(this);
}

History::Parent::iterator History::erase(History::Parent::iterator i) {
	delete *i;
	return Parent::erase(i);
}

void History::blockResized(HistoryBlock *block, int32 dh) {
	int32 i = indexOf(block), l = size();
	if (i >= 0) {
		for (++i; i < l; ++i) {
			(*this)[i]->y -= dh;
		}
		height -= dh;
	}
}

void History::removeBlock(HistoryBlock *block) {
	int32 i = indexOf(block), h = block->height;
	if (i >= 0) {
		removeAt(i);
		int32 l = size();
		if (i > 0 && l == 1) { // only fake block with date left
			removeBlock((*this)[0]);
			height = 0;
		} else if (h) {
			for (; i < l; ++i) {
				(*this)[i]->y -= h;
			}
			height -= h;
		}
	}
	delete block;
}

int32 HistoryBlock::geomResize(int32 newWidth, int32 *ytransform, bool dontRecountText) {
	int32 y = 0;
	for (iterator i = begin(), e = end(); i != e; ++i) {
		HistoryItem *item = *i;
		bool updTransform = ytransform && (*ytransform >= item->y) && (*ytransform < item->y + item->height());
		if (updTransform) *ytransform -= item->y;
		item->y = y;
		y += item->resize(newWidth, dontRecountText);
		if (updTransform) {
			*ytransform += item->y;
			ytransform = 0;
		}
	}
	height = y;
	return height;
}

void HistoryBlock::clear(bool leaveItems) {
	if (leaveItems) {
		for (Parent::const_iterator i = cbegin(), e = cend(); i != e; ++i) {
			(*i)->detachFast();
		}
	} else {
		for (Parent::const_iterator i = cbegin(), e = cend(); i != e; ++i) {
			delete *i;
		}
	}
	Parent::clear();
}

HistoryBlock::Parent::iterator HistoryBlock::erase(HistoryBlock::Parent::iterator i) {
	delete *i;
	return Parent::erase(i);
}

void HistoryBlock::removeItem(HistoryItem *item) {
	int32 i = indexOf(item), dh = 0;
	if (history->showFrom == item) {
		history->getNextShowFrom(this, i);
	}
	if (i < 0) {
		return;
	}

	bool createInitialDate = false;
	QDateTime initialDateTime;
	int32 myIndex = history->indexOf(this);
	if (myIndex >= 0 && item->itemType() != HistoryItem::DateType) { // fix date items
		HistoryItem *nextItem = (i < size() - 1) ? (*this)[i + 1] : ((myIndex < history->size() - 1) ? (*(*history)[myIndex + 1])[0] : 0);
		if (nextItem && nextItem == history->unreadBar) { // skip unread bar
			if (i < size() - 2) {
				nextItem = (*this)[i + 2];
			} else if (i < size() - 1) {
				nextItem = ((myIndex < history->size() - 1) ? (*(*history)[myIndex + 1])[0] : 0);
			} else if (myIndex < history->size() - 1) {
				if (0 < (*history)[myIndex + 1]->size() - 1) {
					nextItem = (*(*history)[myIndex + 1])[1];
				} else if (myIndex < history->size() - 2) {
					nextItem = (*(*history)[myIndex + 2])[0];
				} else {
					nextItem = 0;
				}
			} else {
				nextItem = 0;
			}
		}
		if (!nextItem || nextItem->itemType() == HistoryItem::DateType) { // only if there is no next item or it is a date item
			HistoryItem *prevItem = (i > 0) ? (*this)[i - 1] : 0;
			if (prevItem && prevItem == history->unreadBar) { // skip unread bar
				prevItem = (i > 1) ? (*this)[i - 2] : 0;
			}
			if (prevItem) {
				if (prevItem->itemType() == HistoryItem::DateType) {
					prevItem->destroy();
					--i;
				}
			} else if (myIndex > 0) {
				HistoryBlock *prevBlock = (*history)[myIndex - 1];
				if (prevBlock->isEmpty() || ((myIndex == 1) && (prevBlock->size() != 1 || (*prevBlock->cbegin())->itemType() != HistoryItem::DateType))) {
					LOG(("App Error: Found bad history, with no first date block: %1").arg((*history)[0]->size()));
				} else if ((*prevBlock)[prevBlock->size() - 1]->itemType() == HistoryItem::DateType) {
					(*prevBlock)[prevBlock->size() - 1]->destroy();
					if (nextItem && myIndex == 1) { // destroy next date (for creating initial then)
						initialDateTime = nextItem->date;
						createInitialDate = true;
						nextItem->destroy();
					}
				}
			}
		}
	}
	// myIndex can be invalid now, because of destroying previous blocks

	dh = item->height();
	remove(i);
	int32 l = size();
	if (!item->out() && item->unread() && history->unreadCount) {
		history->setUnreadCount(history->unreadCount - 1);
	}
	int32 itemType = item->itemType();
	if (itemType == HistoryItem::MsgType) {
		history->setMsgCount(history->msgCount - 1);
	} else if (itemType == HistoryItem::UnreadBarType) {
		if (history->unreadBar == item) {
			history->unreadBar = 0;
		}
	}
	if (createInitialDate) {
		history->createInitialDateBlock(initialDateTime);
	}
	History *h = history;
	if (l) {
		for (; i < l; ++i) {
			(*this)[i]->y -= dh;
		}
		height -= dh;
		history->blockResized(this, dh);
	} else {
		history->removeBlock(this);
	}
}

bool ItemAnimations::animStep(float64 ms) {
	for (Animations::iterator i = _animations.begin(); i != _animations.end();) {
		const HistoryItem *item = i.key();
		if (item->animating()) {
			App::main()->msgUpdated(item->history()->peer->id, item);
			++i;
		} else {
			i = _animations.erase(i);
		}
	}
	return !_animations.isEmpty();
}

uint64 ItemAnimations::animate(const HistoryItem *item, uint64 ms) {
	if (_animations.isEmpty()) {
		_animations.insert(item, ms);
		anim::start(this);
		return 0;
	}
	Animations::const_iterator i = _animations.constFind(item);
	if (i == _animations.cend()) i = _animations.insert(item, ms);
	return ms - i.value();
}

void ItemAnimations::remove(const HistoryItem *item) {
	_animations.remove(item);
}

namespace {
	ItemAnimations _itemAnimations;
}

ItemAnimations &itemAnimations() {
	return _itemAnimations;
}

HistoryItem::HistoryItem(History *history, HistoryBlock *block, MsgId msgId, int32 flags, QDateTime msgDate, int32 from) : y(0)
, id(msgId)
, date(msgDate)
, _from(App::user(from))
, _fromVersion(_from->nameVersion)
, _history(history)
, _block(block)
, _flags(flags)
{
}

void HistoryItem::markRead() {
	if (_flags & MTPDmessage_flag_unread) {
		if (out()) {
			_history->outboxRead(this);
		} else {
			_history->inboxRead(this);
		}
		App::main()->msgUpdated(_history->peer->id, this);
		_flags &= ~int32(MTPDmessage_flag_unread);
	}
}

void HistoryItem::destroy() {
	if (!out()) markRead();
	bool wasAtBottom = history()->loadedAtBottom();
	_history->removeNotification(this);
	detach();
	if (history()->lastMsg == this) {
		history()->fixLastMessage(wasAtBottom);
	}
	HistoryMedia *m = getMedia(true);
	MediaOverviewType t = m ? mediaToOverviewType(m->type()) : OverviewCount;
	if (t != OverviewCount) {
		history()->eraseFromOverview(t, id);
		if (m->type() == MediaTypeDocument && static_cast<HistoryDocument*>(m)->document()->song()) {
			history()->eraseFromOverview(OverviewAudioDocuments, id);
		}
	}
	if (hasTextLinks()) {
		history()->eraseFromOverview(OverviewLinks, id);
	}
	delete this;
}

void HistoryItem::detach() {
	if (_history && _history->unreadBar == this) {
		_history->unreadBar = 0;
	}
	if (_block) {
		_block->removeItem(this);
		detachFast();
		App::historyItemDetached(this);
	} else {
		if (_history->showFrom == this) {
			_history->showFrom = 0;
		}
	}
	if (_history && _history->unreadBar && _history->back()->back() == _history->unreadBar) {
		_history->unreadBar->destroy();
	}
}

void HistoryItem::detachFast() {
	_block = 0;
}

HistoryItem::~HistoryItem() {
	itemAnimations().remove(this);
	App::historyUnregItem(this);
	if (id < 0) {
		App::app()->uploader()->cancel(id);
	}
}

HistoryItem *regItem(HistoryItem *item, bool returnExisting) {
	if (!item) return 0;

	HistoryItem *existing = App::historyRegItem(item);
	if (existing) {
		delete item;
		return returnExisting ? existing : 0;
	}

	item->initDimensions();
	return item;
}

HistoryPhoto::HistoryPhoto(const MTPDphoto &photo, const QString &caption, HistoryItem *parent) : HistoryMedia()
, pixw(1), pixh(1)
, data(App::feedPhoto(photo))
, _caption(st::minPhotoSize)
, openl(new PhotoLink(data)) {
	if (!caption.isEmpty()) {
		_caption.setText(st::msgFont, caption + textcmdSkipBlock(parent->timeWidth(true), st::msgDateFont->height - st::msgDateDelta.y()), itemTextParseOptions(parent));
	}
	init();
}

HistoryPhoto::HistoryPhoto(PeerData *chat, const MTPDphoto &photo, int32 width) : HistoryMedia(width)
, pixw(1), pixh(1)
, data(App::feedPhoto(photo))
, openl(new PhotoLink(data, chat)) {
	init();
}

void HistoryPhoto::init() {
	data->thumb->load();
}

void HistoryPhoto::initDimensions(const HistoryItem *parent) {
	int32 tw = convertScale(data->full->width()), th = convertScale(data->full->height());
	if (!tw || !th) {
		tw = th = 1;
	}
	int32 thumbw = tw;
	int32 thumbh = th;
	if (!w) {
		w = thumbw;
	} else {
		thumbh = w; // square chat photo updates
	}
	_maxw = qMax(w, int32(st::minPhotoSize));
	_minh = qMax(thumbh, int32(st::minPhotoSize));
	const HistoryReply *reply = toHistoryReply(parent);
	const HistoryForwarded *fwd = toHistoryForwarded(parent);
	if (reply || !_caption.isEmpty()) {
		_maxw += st::mediaPadding.left() + st::mediaPadding.right();
		if (reply) {
			_minh += st::msgReplyPadding.top() + st::msgReplyBarSize.height() + st::msgReplyPadding.bottom();
		} else {
			if (parent->out() || !parent->history()->peer->chat || !fwd) {
				_minh += st::msgPadding.top();
			}
			if (fwd) {
				_minh += st::msgServiceNameFont->height + st::msgPadding.top();
			}
		}
		if (!parent->out() && parent->history()->peer->chat) {
			_minh += st::msgPadding.top() + st::msgNameFont->height;
		}
		if (!_caption.isEmpty()) {
			_minh += st::webPagePhotoSkip + _caption.minHeight();
		}
		_minh += st::mediaPadding.bottom();
	}
}

int32 HistoryPhoto::resize(int32 width, bool dontRecountText, const HistoryItem *parent) {
	const HistoryReply *reply = toHistoryReply(parent);
	const HistoryForwarded *fwd = reply ? 0 : toHistoryForwarded(parent);

	pixw = qMin(width, _maxw);
	if (reply || !_caption.isEmpty()) {
		pixw -= st::mediaPadding.left() + st::mediaPadding.right();
	}

	int32 tw = convertScale(data->full->width()), th = convertScale(data->full->height());
	if (tw > st::maxMediaSize) {
		th = (st::maxMediaSize * th) / tw;
		tw = st::maxMediaSize;
	}
	if (th > st::maxMediaSize) {
		tw = (st::maxMediaSize * tw) / th;
		th = st::maxMediaSize;
	}
	pixh = th;
	if (tw > pixw) {
		pixh = (pixw * pixh / tw);
	} else {
		pixw = tw;
	}
	if (pixh > width) {
		pixw = (pixw * width) / pixh;
		pixh = width;
	}
	if (pixw < 1) pixw = 1;
	if (pixh < 1) pixh = 1;
	w = qMax(pixw, int16(st::minPhotoSize));
	_height = qMax(pixh, int16(st::minPhotoSize));
	if (reply || !_caption.isEmpty()) {
		if (reply) {
			_height += st::msgReplyPadding.top() + st::msgReplyBarSize.height() + st::msgReplyPadding.bottom();
		} else {
			if (parent->out() || !parent->history()->peer->chat || !fwd) {
				_height += st::msgPadding.top();
			}
			if (fwd) {
				_height += st::msgServiceNameFont->height + st::msgPadding.top();
			}
		}
		if (!parent->out() && parent->history()->peer->chat) {
			_height += st::msgPadding.top() + st::msgNameFont->height;
		}
		if (!_caption.isEmpty()) {
			_height += st::webPagePhotoSkip + _caption.countHeight(w);
		}
		_height += st::mediaPadding.bottom();
		w += st::mediaPadding.left() + st::mediaPadding.right();
	}
	return _height;
}

const QString HistoryPhoto::inDialogsText() const {
	return lang(lng_in_dlg_photo);
}

const QString HistoryPhoto::inHistoryText() const {
	return qsl("[ ") + lang(lng_in_dlg_photo) + qsl(" ]");
}

const Text &HistoryPhoto::captionForClone() const {
	return _caption;
}

bool HistoryPhoto::hasPoint(int32 x, int32 y, const HistoryItem *parent, int32 width) const {
	if (width < 0) width = w;
	return (x >= 0 && y >= 0 && x < width && y < _height);
}

void HistoryPhoto::getState(TextLinkPtr &lnk, HistoryCursorState &state, int32 x, int32 y, const HistoryItem *parent, int32 width) const {
	if (width < 0) width = w;
	if (width < 1) return;

	int skipx = 0, skipy = 0, height = _height;
	const HistoryReply *reply = toHistoryReply(parent);
	const HistoryForwarded *fwd = reply ? 0 : toHistoryForwarded(parent);
	int replyFrom = 0, fwdFrom = 0;
	if (reply || !_caption.isEmpty()) {
		skipx = st::mediaPadding.left();
		if (reply) {
			skipy = st::msgReplyPadding.top() + st::msgReplyBarSize.height() + st::msgReplyPadding.bottom();
		} else if (fwd) {
			skipy = st::msgServiceNameFont->height + st::msgPadding.top();
		}
		if (!parent->out() && parent->history()->peer->chat) {
			replyFrom = st::msgPadding.top() + st::msgNameFont->height;
			fwdFrom = st::msgPadding.top() + st::msgNameFont->height;
			skipy += replyFrom;
			if (x >= st::mediaPadding.left() && y >= st::msgPadding.top() && x < width - st::mediaPadding.left() - st::mediaPadding.right() && x < st::mediaPadding.left() + parent->from()->nameText.maxWidth() && y < replyFrom) {
				lnk = parent->from()->lnk;
				return;
			}
			if (!reply && !fwd) skipy += st::msgPadding.top();
		} else if (!reply) {
			fwdFrom = st::msgPadding.top();
			skipy += fwdFrom;
		}
		if (reply) {
			if (x >= 0 && y >= replyFrom + st::msgReplyPadding.top() && x < width && y < skipy - st::msgReplyPadding.bottom()) {
				lnk = reply->replyToLink();
				return;
			}
		} else if (fwd) {
			if (y >= fwdFrom && y < fwdFrom + st::msgServiceNameFont->height) {
				return fwd->getForwardedState(lnk, state, x - st::mediaPadding.left(), width - st::mediaPadding.left() - st::mediaPadding.right());
			}
		}
		height -= skipy + st::mediaPadding.bottom();
		width -= st::mediaPadding.left() + st::mediaPadding.right();
		if (!_caption.isEmpty()) {
			int32 dateX = skipx + width + st::msgDateDelta.x() - parent->timeWidth(true) + st::msgDateSpace;
			int32 dateY = _height - st::msgPadding.bottom() + st::msgDateDelta.y() - st::msgDateFont->height;
			bool inDate = QRect(dateX, dateY, parent->timeWidth(true) - st::msgDateSpace, st::msgDateFont->height).contains(x, y);
			if (inDate) {
				state = HistoryInDateCursorState;
			}

			height -= _caption.countHeight(width) + st::webPagePhotoSkip;
			if (x >= skipx && y >= skipy + height + st::webPagePhotoSkip && x < skipx + width && y < _height) {
				bool inText = false;
				_caption.getState(lnk, inText, x - skipx, y - skipy - height - st::webPagePhotoSkip, width);
				state = inDate ? HistoryInDateCursorState : (inText ? HistoryInTextCursorState : HistoryDefaultCursorState);
			}
		}
	}
	if (x >= skipx && y >= skipy && x < skipx + width && y < skipy + height) {
		lnk = openl;
		if (_caption.isEmpty()) {
			int32 dateX = skipx + width - parent->timeWidth(false) - st::msgDateImgDelta - st::msgDateImgPadding.x();
			int32 dateY = skipy + height - st::msgDateFont->height - st::msgDateImgDelta - st::msgDateImgPadding.y();
			if (parent->out()) {
				dateX -= st::msgCheckRect.pxWidth() + st::msgDateImgCheckSpace;
			}
			bool inDate = QRect(dateX, dateY, parent->timeWidth(true) - st::msgDateSpace, st::msgDateFont->height).contains(x, y);
			if (inDate) {
				state = HistoryInDateCursorState;
			}
		}
		return;
	}
}

HistoryMedia *HistoryPhoto::clone() const {
	return new HistoryPhoto(*this);
}

void HistoryPhoto::updateFrom(const MTPMessageMedia &media) {
	if (media.type() == mtpc_messageMediaPhoto) {
		const MTPPhoto &photo(media.c_messageMediaPhoto().vphoto);
		if (photo.type() == mtpc_photo) {
			const QVector<MTPPhotoSize> &sizes(photo.c_photo().vsizes.c_vector().v);
			for (QVector<MTPPhotoSize>::const_iterator i = sizes.cbegin(), e = sizes.cend(); i != e; ++i) {
				char size = 0;
				const MTPFileLocation *loc = 0;
				switch (i->type()) {
				case mtpc_photoSize: {
					const string &s(i->c_photoSize().vtype.c_string().v);
					loc = &i->c_photoSize().vlocation;
					if (s.size()) size = s[0];
				} break;

				case mtpc_photoCachedSize: {
					const string &s(i->c_photoCachedSize().vtype.c_string().v);
					loc = &i->c_photoCachedSize().vlocation;
					if (s.size()) size = s[0];
				} break;
				}
				if (!loc || loc->type() != mtpc_fileLocation) continue;
				if (size == 's') {
					Local::writeImage(storageKey(loc->c_fileLocation()), data->thumb);
				} else if (size == 'm') {
					Local::writeImage(storageKey(loc->c_fileLocation()), data->medium);
				} else if (size == 'x') {
					Local::writeImage(storageKey(loc->c_fileLocation()), data->full);
				}
			}
		}
	}
}

void HistoryPhoto::draw(QPainter &p, const HistoryItem *parent, bool selected, int32 width) const {
	const HistoryReply *reply = toHistoryReply(parent);
	const HistoryForwarded *fwd = reply ? 0 : toHistoryForwarded(parent);
	bool out = parent->out();

	if (width < 0) width = w;
	int skipx = 0, skipy = 0, height = _height;
	if (reply || !_caption.isEmpty()) {
		skipx = st::mediaPadding.left();

		style::color bg(selected ? (out ? st::msgOutSelectBg : st::msgInSelectBg) : (out ? st::msgOutBg : st::msgInBg));
		style::color sh(selected ? (out ? st::msgOutSelectShadow : st::msgInSelectShadow) : (out ? st::msgOutShadow : st::msgInShadow));
		RoundCorners cors(selected ? (out ? MessageOutSelectedCorners : MessageInSelectedCorners) : (out ? MessageOutCorners : MessageInCorners));
		App::roundRect(p, 0, 0, width, _height, bg, cors, &sh);

		int replyFrom = 0, fwdFrom = 0;
		if (!out && parent->history()->peer->chat) {
			replyFrom = st::msgPadding.top() + st::msgNameFont->height;
			fwdFrom = st::msgPadding.top() + st::msgNameFont->height;
			skipy += replyFrom;
			p.setFont(st::msgNameFont->f);
			p.setPen(parent->from()->color->p);
			parent->from()->nameText.drawElided(p, st::mediaPadding.left(), st::msgPadding.top(), width - st::mediaPadding.left() - st::mediaPadding.right());
			if (!fwd && !reply) skipy += st::msgPadding.top();
		} else if (!reply) {
			fwdFrom = st::msgPadding.top();
			skipy += fwdFrom;
		}
		if (reply) {
			skipy += st::msgReplyPadding.top() + st::msgReplyBarSize.height() + st::msgReplyPadding.bottom();
			reply->drawReplyTo(p, st::msgReplyPadding.left(), replyFrom, width - st::msgReplyPadding.left() - st::msgReplyPadding.right(), selected);
		} else if (fwd) {
			skipy += st::msgServiceNameFont->height + st::msgPadding.top();
			fwd->drawForwardedFrom(p, st::mediaPadding.left(), fwdFrom, width - st::mediaPadding.left() - st::mediaPadding.right(), selected);
		}
		width -= st::mediaPadding.left() + st::mediaPadding.right();
		height -= skipy + st::mediaPadding.bottom();
		if (!_caption.isEmpty()) {
			height -= st::webPagePhotoSkip + _caption.countHeight(width);
		}
	} else {
		App::roundShadow(p, 0, 0, width, _height, selected ? st::msgInSelectShadow : st::msgInShadow, selected ? InSelectedShadowCorners : InShadowCorners);
	}
	data->full->load(false, false);
	bool full = data->full->loaded();
	QPixmap pix;
	if (full) {
		pix = data->full->pixSingle(pixw, pixh, width, height);
	} else {
		pix = data->thumb->pixBlurredSingle(pixw, pixh, width, height);
	}
	p.drawPixmap(skipx, skipy, pix);
	if (!full) {
		uint64 dt = itemAnimations().animate(parent, getms());
		int32 cnt = int32(st::photoLoaderCnt), period = int32(st::photoLoaderPeriod), t = dt % period, delta = int32(st::photoLoaderDelta);

		int32 x = (width - st::photoLoader.width()) / 2, y = (height - st::photoLoader.height()) / 2;
		p.fillRect(skipx + x, skipy + y, st::photoLoader.width(), st::photoLoader.height(), st::photoLoaderBg->b);
		x += (st::photoLoader.width() - cnt * st::photoLoaderPoint.width() - (cnt - 1) * st::photoLoaderSkip) / 2;
		y += (st::photoLoader.height() - st::photoLoaderPoint.height()) / 2;
		QColor c(st::white->c);
		QBrush b(c);
		for (int32 i = 0; i < cnt; ++i) {
			t -= delta;
			while (t < 0) t += period;
				
			float64 alpha = (t >= st::photoLoaderDuration1 + st::photoLoaderDuration2) ? 0 : ((t > st::photoLoaderDuration1 ? ((st::photoLoaderDuration1 + st::photoLoaderDuration2 - t) / st::photoLoaderDuration2) : (t / st::photoLoaderDuration1)));
			c.setAlphaF(st::photoLoaderAlphaMin + alpha * (1 - st::photoLoaderAlphaMin));
			b.setColor(c);
			p.fillRect(skipx + x + i * (st::photoLoaderPoint.width() + st::photoLoaderSkip), skipy + y, st::photoLoaderPoint.width(), st::photoLoaderPoint.height(), b);
		}
	}

	if (selected) {
		App::roundRect(p, skipx, skipy, width, height, textstyleCurrent()->selectOverlay, SelectedOverlayCorners);
	}

	// date
	QString time(parent->time());
	if (_caption.isEmpty()) {
		if (time.isEmpty()) return;
		int32 dateX = skipx + width - parent->timeWidth(false) - st::msgDateImgDelta - 2 * st::msgDateImgPadding.x();
		int32 dateY = skipy + height - st::msgDateFont->height - 2 * st::msgDateImgPadding.y() - st::msgDateImgDelta;
		if (parent->out()) {
			dateX -= st::msgCheckRect.pxWidth() + st::msgDateImgCheckSpace;
		}
		int32 dateW = skipx + width - dateX - st::msgDateImgDelta;
		int32 dateH = skipy + height - dateY - st::msgDateImgDelta;

		App::roundRect(p, dateX, dateY, dateW, dateH, selected ? st::msgDateImgSelectBg : st::msgDateImgBg, selected ? DateSelectedCorners : DateCorners);

		p.setFont(st::msgDateFont->f);
		p.setPen(st::msgDateImgColor->p);
		p.drawText(dateX + st::msgDateImgPadding.x(), dateY + st::msgDateImgPadding.y() + st::msgDateFont->ascent, time);
		if (out) {
			QPoint iconPos(dateX - 2 + dateW - st::msgDateImgCheckSpace - st::msgCheckRect.pxWidth(), dateY + (dateH - st::msgCheckRect.pxHeight()) / 2);
			const QRect *iconRect;
			if (parent->id > 0) {
				if (parent->unread()) {
					iconRect = &st::msgImgCheckRect;
				} else {
					iconRect = &st::msgImgDblCheckRect;
				}
			} else {
				iconRect = &st::msgImgSendingRect;
			}
			p.drawPixmap(iconPos, App::sprite(), *iconRect);
		}
	} else {
		p.setPen(st::black->p);
		_caption.draw(p, skipx, skipy + height + st::webPagePhotoSkip, width);

		style::color date(selected ? (out ? st::msgOutSelectDateColor : st::msgInSelectDateColor) : (out ? st::msgOutDateColor : st::msgInDateColor));
		p.setPen(date->p);

		p.drawText(w - st::msgPadding.right() + st::msgDateDelta.x() - parent->timeWidth(true) + st::msgDateSpace, _height - st::msgPadding.bottom() + st::msgDateDelta.y() - st::msgDateFont->descent, time);
		if (out) {
			QPoint iconPos(w + st::msgCheckPos.x() - st::msgPadding.right() - st::msgCheckRect.pxWidth(), _height + st::msgCheckPos.y() - st::msgPadding.bottom() + st::msgDateDelta.y() - st::msgCheckRect.pxHeight());
			const QRect *iconRect;
			if (parent->id > 0) {
				if (parent->unread()) {
					iconRect = &(selected ? st::msgSelectCheckRect : st::msgCheckRect);
				} else {
					iconRect = &(selected ? st::msgSelectDblCheckRect : st::msgDblCheckRect);
				}
			} else {
				iconRect = &st::msgSendingRect;
			}
			p.drawPixmap(iconPos, App::sprite(), *iconRect);
		}
	}
}

ImagePtr HistoryPhoto::replyPreview() {
	return data->makeReplyPreview();
}

QString formatSizeText(qint64 size) {
	if (size >= 1024 * 1024) { // more than 1 mb
		qint64 sizeTenthMb = (size * 10 / (1024 * 1024));
		return QString::number(sizeTenthMb / 10) + '.' + QString::number(sizeTenthMb % 10) + qsl(" MB");
	}
	qint64 sizeTenthKb = (size * 10 / 1024);
	return QString::number(sizeTenthKb / 10) + '.' + QString::number(sizeTenthKb % 10) + qsl(" KB");
}

QString formatDownloadText(qint64 ready, qint64 total) {
	QString readyStr, totalStr, mb;
	if (total >= 1024 * 1024) { // more than 1 mb
		qint64 readyTenthMb = (ready * 10 / (1024 * 1024)), totalTenthMb = (total * 10 / (1024 * 1024));
		readyStr = QString::number(readyTenthMb / 10) + '.' + QString::number(readyTenthMb % 10);
		totalStr = QString::number(totalTenthMb / 10) + '.' + QString::number(totalTenthMb % 10);
		mb = qsl("MB");
	} else {
		qint64 readyKb = (ready / 1024), totalKb = (total / 1024);
		readyStr = QString::number(readyKb);
		totalStr = QString::number(totalKb);
		mb = qsl("KB");
	}
	return lng_save_downloaded(lt_ready, readyStr, lt_total, totalStr, lt_mb, mb);
}

QString formatDurationText(qint64 duration) {
	qint64 hours = (duration / 3600), minutes = (duration % 3600) / 60, seconds = duration % 60;
	return (hours ? QString::number(hours) + ':' : QString()) + (minutes >= 10 ? QString() : QString('0')) + QString::number(minutes) + ':' + (seconds >= 10 ? QString() : QString('0')) + QString::number(seconds);
}

QString formatDurationAndSizeText(qint64 duration, qint64 size) {
	return lng_duration_and_size(lt_duration, formatDurationText(duration), lt_size, formatSizeText(size));
}

int32 _downloadWidth = 0, _openWithWidth = 0, _cancelWidth = 0, _buttonWidth = 0;

HistoryVideo::HistoryVideo(const MTPDvideo &video, const QString &caption, HistoryItem *parent) : HistoryMedia()
, data(App::feedVideo(video))
, _openl(new VideoOpenLink(data))
, _savel(new VideoSaveLink(data))
, _cancell(new VideoCancelLink(data))
, _caption(st::minPhotoSize)
, _dldDone(0)
, _uplDone(0)
{
	if (!caption.isEmpty()) {
		_caption.setText(st::msgFont, caption + textcmdSkipBlock(parent->timeWidth(true), st::msgDateFont->height - st::msgDateDelta.y()), itemTextParseOptions(parent));
	}

	_size = formatDurationAndSizeText(data->duration, data->size);

	if (!_openWithWidth) {
		_downloadWidth = st::mediaSaveButton.font->m.width(lang(lng_media_download));
		_openWithWidth = st::mediaSaveButton.font->m.width(lang(lng_media_open_with));
		_cancelWidth = st::mediaSaveButton.font->m.width(lang(lng_media_cancel));
		_buttonWidth = (st::mediaSaveButton.width > 0) ? st::mediaSaveButton.width : ((_downloadWidth > _openWithWidth ? (_downloadWidth > _cancelWidth ? _downloadWidth : _cancelWidth) : _openWithWidth) - st::mediaSaveButton.width);
	}

	data->thumb->load();

	int32 tw = data->thumb->width(), th = data->thumb->height();
	if (data->thumb->isNull() || !tw || !th) {
		_thumbw = 0;
	} else if (tw > th) {
		_thumbw = (tw * st::mediaThumbSize) / th;
	} else {
		_thumbw = st::mediaThumbSize;
	}
}

void HistoryVideo::initDimensions(const HistoryItem *parent) {
	_maxw = st::mediaMaxWidth;
	int32 tleft = st::mediaPadding.left() + st::mediaThumbSize + st::mediaPadding.right();
	if (!parent->out()) { // add Download / Save As button
		_maxw += st::mediaSaveDelta + _buttonWidth;
	}
	_minh = st::mediaPadding.top() + st::mediaThumbSize + st::mediaPadding.bottom();
	if (!parent->out() && parent->history()->peer->chat) {
		_minh += st::msgPadding.top() + st::msgNameFont->height;
	}
	if (const HistoryReply *reply = toHistoryReply(parent)) {
		_minh += st::msgReplyPadding.top() + st::msgReplyBarSize.height() + st::msgReplyPadding.bottom();
	} else if (const HistoryForwarded *fwd = toHistoryForwarded(parent)) {
		if (parent->out() || !parent->history()->peer->chat) {
			_minh += st::msgPadding.top();
		}
		_minh += st::msgServiceNameFont->height;
	}
	if (_caption.isEmpty()) {
		_height = _minh;
	} else {
		_minh += st::webPagePhotoSkip + _caption.minHeight();
	}
}

void HistoryVideo::regItem(HistoryItem *item) {
	App::regVideoItem(data, item);
}

void HistoryVideo::unregItem(HistoryItem *item) {
	App::unregVideoItem(data, item);
}

const QString HistoryVideo::inDialogsText() const {
	return lang(lng_in_dlg_video);
}

const QString HistoryVideo::inHistoryText() const {
	return qsl("[ ") + lang(lng_in_dlg_video) + qsl(" ]");
}

bool HistoryVideo::hasPoint(int32 x, int32 y, const HistoryItem *parent, int32 width) const {
	int32 height = _height;
	if (width < 0) {
		width = w;
	} else if (!_caption.isEmpty()) {
		height = countHeight(parent, width);
	}
	if (width >= _maxw) {
		width = _maxw;
	}
	return (x >= 0 && y >= 0 && x < width && y < height);
}

int32 HistoryVideo::countHeight(const HistoryItem *parent, int32 width) const {
	if (_caption.isEmpty()) return _height;

	if (width < 0) width = w;
	if (width >= _maxw) {
		width = _maxw;
	}

	int32 h = st::mediaPadding.top() + st::mediaThumbSize + st::mediaPadding.bottom();
	if (!parent->out() && parent->history()->peer->chat) {
		h += st::msgPadding.top() + st::msgNameFont->height;
	}
	if (const HistoryReply *reply = toHistoryReply(parent)) {
		h += st::msgReplyPadding.top() + st::msgReplyBarSize.height() + st::msgReplyPadding.bottom();
	} else if (const HistoryForwarded *fwd = toHistoryForwarded(parent)) {
		if (parent->out() || !parent->history()->peer->chat) {
			h += st::msgPadding.top();
		}
		h += st::msgServiceNameFont->height;
	}
	if (!_caption.isEmpty()) {
		int32 textw = width - st::mediaPadding.left() - st::mediaPadding.right();
		if (!parent->out()) { // substract Download / Save As button
			textw -= st::mediaSaveDelta + _buttonWidth;
		}
		h += st::webPagePhotoSkip + _caption.countHeight(textw);
	}
	return h;
}

void HistoryVideo::getState(TextLinkPtr &lnk, HistoryCursorState &state, int32 x, int32 y, const HistoryItem *parent, int32 width) const {
	int32 height = _height;
	if (width < 0) {
		width = w;
	} else if (!_caption.isEmpty()) {
		height = countHeight(parent, width);
	}
	if (width < 1) return;

	const HistoryReply *reply = toHistoryReply(parent);
	const HistoryForwarded *fwd = reply ? 0 : toHistoryForwarded(parent);
	int skipy = 0, replyFrom = 0, fwdFrom = 0;
	if (reply) {
		skipy = st::msgReplyPadding.top() + st::msgReplyBarSize.height() + st::msgReplyPadding.bottom();
	} else if (fwd) {
		skipy = st::msgServiceNameFont->height;
	}
	if (!parent->out() && parent->history()->peer->chat) {
		replyFrom = st::msgPadding.top() + st::msgNameFont->height;
		fwdFrom = st::msgPadding.top() + st::msgNameFont->height;
		skipy += replyFrom;
	} else if (fwd) {
		fwdFrom = st::msgPadding.top();
		skipy += fwdFrom;
	}

	bool out = parent->out(), hovered, pressed;
	if (width >= _maxw) {
		width = _maxw;
	}

	if (!out) { // draw Download / Save As button
		int32 h = height;
		if (!_caption.isEmpty()) {
			h -= st::webPagePhotoSkip + _caption.countHeight(width - _buttonWidth - st::mediaSaveDelta - st::mediaPadding.left() - st::mediaPadding.right());
		}
		int32 btnw = _buttonWidth, btnh = st::mediaSaveButton.height, btnx = width - _buttonWidth, btny = skipy + (h - skipy - btnh) / 2;
		if (x >= btnx && y >= btny && x < btnx + btnw && y < btny + btnh) {
			lnk = data->loader ? _cancell : _savel;
			return;
		}
		width -= btnw + st::mediaSaveDelta;
	}

	if (!parent->out() && parent->history()->peer->chat) {
		if (x >= st::mediaPadding.left() && y >= st::msgPadding.top() && x < width - st::mediaPadding.left() - st::mediaPadding.right() && x < st::mediaPadding.left() + parent->from()->nameText.maxWidth() && y < replyFrom) {
			lnk = parent->from()->lnk;
			return;
		}
	}
	if (reply) {
		if (x >= 0 && y >= replyFrom + st::msgReplyPadding.top() && x < width && y < skipy - st::msgReplyPadding.bottom()) {
			lnk = reply->replyToLink();
			return;
		}
	} else if (fwd) {
		if (y >= fwdFrom && y < skipy) {
			return fwd->getForwardedState(lnk, state, x - st::mediaPadding.left(), width - st::mediaPadding.left() - st::mediaPadding.right());
		}
	}

	int32 dateX = width - st::msgPadding.right() + st::msgDateDelta.x() - parent->timeWidth(true) + st::msgDateSpace;
	int32 dateY = height - st::msgPadding.bottom() + st::msgDateDelta.y() - st::msgDateFont->height;
	bool inDate = QRect(dateX, dateY, parent->timeWidth(true) - st::msgDateSpace, st::msgDateFont->height).contains(x, y);
	if (inDate) {
		state = HistoryInDateCursorState;
	}

	int32 tw = width - st::mediaPadding.left() - st::mediaPadding.right();
	if (x >= st::mediaPadding.left() && y >= skipy + st::mediaPadding.top() && x < st::mediaPadding.left() + tw && y < skipy + st::mediaPadding.top() + st::mediaThumbSize && !data->loader && data->access) {
		lnk = _openl;
		return;
	}
	if (!_caption.isEmpty() && x >= st::mediaPadding.left() && x < st::mediaPadding.left() + tw && y >= skipy + st::mediaPadding.top() + st::mediaThumbSize + st::webPagePhotoSkip) {
		bool inText = false;
		_caption.getState(lnk, inText, x - st::mediaPadding.left(), y - skipy - st::mediaPadding.top() - st::mediaThumbSize - st::webPagePhotoSkip, tw);
		state = inDate ? HistoryInDateCursorState : (inText ? HistoryInTextCursorState : HistoryDefaultCursorState);
	}
}

HistoryMedia *HistoryVideo::clone() const {
	return new HistoryVideo(*this);
}

void HistoryVideo::draw(QPainter &p, const HistoryItem *parent, bool selected, int32 width) const {
	int32 height = _height;
	if (width < 0) {
		width = w;
	} else if (!_caption.isEmpty()) {
		height = countHeight(parent, width);
	}
	if (width < 1) return;

	const HistoryReply *reply = toHistoryReply(parent);
	const HistoryForwarded *fwd = reply ? 0 : toHistoryForwarded(parent);
	int skipy = 0, replyFrom = 0, fwdFrom = 0;
	if (reply) {
		skipy = st::msgReplyPadding.top() + st::msgReplyBarSize.height() + st::msgReplyPadding.bottom();
	} else if (fwd) {
		skipy = st::msgServiceNameFont->height;
	}
	if (!parent->out() && parent->history()->peer->chat) {
		replyFrom = st::msgPadding.top() + st::msgNameFont->height;
		fwdFrom = st::msgPadding.top() + st::msgNameFont->height;
		skipy += replyFrom;
	} else if (fwd) {
		fwdFrom = st::msgPadding.top();
		skipy += fwdFrom;
	}

	data->thumb->checkload();

	bool out = parent->out(), hovered, pressed;
	if (width >= _maxw) {
		width = _maxw;
	}

	if (!out) { // draw Download / Save As button
		hovered = ((data->loader ? _cancell : _savel) == textlnkOver());
		pressed = hovered && ((data->loader ? _cancell : _savel) == textlnkDown());
		if (hovered && !pressed && textlnkDown()) hovered = false;

		int32 h = height;
		if (!_caption.isEmpty()) {
			h -= st::webPagePhotoSkip + _caption.countHeight(width - _buttonWidth - st::mediaSaveDelta - st::mediaPadding.left() - st::mediaPadding.right());
		}

		int32 btnw = _buttonWidth, btnh = st::mediaSaveButton.height, btnx = width - _buttonWidth, btny = skipy + (h - skipy - btnh) / 2;
		style::color bg(selected ? st::msgInSelectBg : (hovered ? st::mediaSaveButton.overBgColor : st::mediaSaveButton.bgColor));
		style::color sh(selected ? st::msgInSelectShadow : st::msgInShadow);
		RoundCorners cors(selected ? MessageInSelectedCorners : (hovered ? ButtonHoverCorners : MessageInCorners));
		App::roundRect(p, btnx, btny, btnw, btnh, bg, cors, &sh);

		p.setPen((hovered ? st::mediaSaveButton.overColor : st::mediaSaveButton.color)->p);
		p.setFont(st::mediaSaveButton.font->f);
		QString btnText(lang(data->loader ? lng_media_cancel : (data->already().isEmpty() ? lng_media_download : lng_media_open_with)));
		int32 btnTextWidth = data->loader ? _cancelWidth : (data->already().isEmpty() ? _downloadWidth : _openWithWidth);
		p.drawText(btnx + (btnw - btnTextWidth) / 2, btny + (pressed ? st::mediaSaveButton.downTextTop : st::mediaSaveButton.textTop) + st::mediaSaveButton.font->ascent, btnText);
		width -= btnw + st::mediaSaveDelta;
	}

	style::color bg(selected ? (out ? st::msgOutSelectBg : st::msgInSelectBg) : (out ? st::msgOutBg : st::msgInBg));
	style::color sh(selected ? (out ? st::msgOutSelectShadow : st::msgInSelectShadow) : (out ? st::msgOutShadow : st::msgInShadow));
	RoundCorners cors(selected ? (out ? MessageOutSelectedCorners : MessageInSelectedCorners) : (out ? MessageOutCorners : MessageInCorners));
	App::roundRect(p, 0, 0, width, height, bg, cors, &sh);

	if (!parent->out() && parent->history()->peer->chat) {
		p.setFont(st::msgNameFont->f);
		p.setPen(parent->from()->color->p);
		parent->from()->nameText.drawElided(p, st::mediaPadding.left(), st::msgPadding.top(), width - st::mediaPadding.left() - st::mediaPadding.right());
	}
	if (reply) {
		reply->drawReplyTo(p, st::msgReplyPadding.left(), replyFrom, width - st::msgReplyPadding.left() - st::msgReplyPadding.right(), selected);
	} else if (fwd) {
		fwd->drawForwardedFrom(p, st::mediaPadding.left(), fwdFrom, width - st::mediaPadding.left() - st::mediaPadding.right(), selected);
	}

	if (_thumbw) {
		p.drawPixmap(QPoint(st::mediaPadding.left(), skipy + st::mediaPadding.top()), data->thumb->pixSingle(_thumbw, 0, st::mediaThumbSize, st::mediaThumbSize));
	} else {
		p.drawPixmap(QPoint(st::mediaPadding.left(), skipy + st::mediaPadding.top()), App::sprite(), (out ? st::mediaDocOutImg : st::mediaDocInImg));
	}
	if (selected) {
		App::roundRect(p, st::mediaPadding.left(), skipy + st::mediaPadding.top(), st::mediaThumbSize, st::mediaThumbSize, textstyleCurrent()->selectOverlay, SelectedOverlayCorners);
	}

	int32 tleft = st::mediaPadding.left() + st::mediaThumbSize + st::mediaPadding.right();
	int32 twidth = width - tleft - st::mediaPadding.right();
	int32 fullTimeWidth = parent->timeWidth(true) + st::msgPadding.right();
	int32 secondwidth = width - tleft - fullTimeWidth;

	p.setFont(st::mediaFont->f);
	p.setPen(st::black->c);
	p.drawText(tleft, skipy + st::mediaPadding.top() + st::mediaNameTop + st::mediaFont->ascent, lang(lng_media_video));

	QString statusText;

	style::color status(selected ? (out ? st::mediaOutSelectColor : st::mediaInSelectColor) : (out ? st::mediaOutColor : st::mediaInColor));
	p.setPen(status->p);

	if (data->loader) {
		int32 offset = data->loader->currentOffset();
		if (_dldTextCache.isEmpty() || _dldDone != offset) {
			_dldDone = offset;
			_dldTextCache = formatDownloadText(_dldDone, data->size);
		}
		statusText = _dldTextCache;
	} else {
		if (data->status == FileFailed) {
			statusText = lang(lng_attach_failed);
		} else if (data->status == FileUploading) {
			if (_uplTextCache.isEmpty() || _uplDone != data->uploadOffset) {
				_uplDone = data->uploadOffset;
				_uplTextCache = formatDownloadText(_uplDone, data->size);
			}
			statusText = _uplTextCache;
		} else {
			statusText = _size;
		}
	}
	int32 texty = skipy + st::mediaPadding.top() + st::mediaThumbSize - st::mediaDetailsShift - st::mediaFont->height;
	p.drawText(tleft, texty + st::mediaFont->ascent, statusText);
	if (parent->isMediaUnread()) {
		int32 w = st::mediaFont->m.width(statusText);
		if (w + st::mediaUnreadSkip + st::mediaUnreadSize <= twidth) {
			p.setRenderHint(QPainter::HighQualityAntialiasing, true);
			p.setPen(Qt::NoPen);
			p.setBrush((out ? (selected ? st::mediaOutUnreadSelectColor : st::mediaOutUnreadColor) : (selected ? st::mediaInUnreadSelectColor : st::mediaInUnreadColor))->b);
			p.drawEllipse(QRect(tleft + w + st::mediaUnreadSkip, texty + ((st::mediaFont->height - st::mediaUnreadSize) / 2), st::mediaUnreadSize, st::mediaUnreadSize));
			p.setRenderHint(QPainter::HighQualityAntialiasing, false);
		}
	}
	p.setFont(st::msgDateFont->f);

	if (!_caption.isEmpty()) {
		p.setPen(st::black->p);
		_caption.draw(p, st::mediaPadding.left(), skipy + st::mediaPadding.top() + st::mediaThumbSize + st::webPagePhotoSkip, width - st::mediaPadding.left() - st::mediaPadding.right());
	}

	style::color date(selected ? (out ? st::msgOutSelectDateColor : st::msgInSelectDateColor) : (out ? st::msgOutDateColor : st::msgInDateColor));
	p.setPen(date->p);

	p.drawText(width + st::msgDateDelta.x() - fullTimeWidth + st::msgDateSpace, height - st::msgPadding.bottom() + st::msgDateDelta.y() - st::msgDateFont->descent, parent->time());
	if (out) {
		QPoint iconPos(width + 5 - st::msgPadding.right() - st::msgCheckRect.pxWidth(), height + 1 - st::msgPadding.bottom() + st::msgDateDelta.y() - st::msgCheckRect.pxHeight());
		const QRect *iconRect;
		if (parent->id > 0) {
			if (parent->unread()) {
				iconRect = &(selected ? st::msgSelectCheckRect : st::msgCheckRect);
			} else {
				iconRect = &(selected ? st::msgSelectDblCheckRect : st::msgDblCheckRect);
			}
		} else {
			iconRect = &st::msgSendingRect;
		}
		p.drawPixmap(iconPos, App::sprite(), *iconRect);
	}
}

int32 HistoryVideo::resize(int32 width, bool dontRecountText, const HistoryItem *parent) {
	w = qMin(width, _maxw);
	if (_caption.isEmpty()) return _height;

	_height = st::mediaPadding.top() + st::mediaThumbSize + st::mediaPadding.bottom();
	if (!parent->out() && parent->history()->peer->chat) {
		_height += st::msgPadding.top() + st::msgNameFont->height;
	}
	if (const HistoryReply *reply = toHistoryReply(parent)) {
		_height += st::msgReplyPadding.top() + st::msgReplyBarSize.height() + st::msgReplyPadding.bottom();
	} else if (const HistoryForwarded *fwd = toHistoryForwarded(parent)) {
		if (parent->out() || !parent->history()->peer->chat) {
			_height += st::msgPadding.top();
		}
		_height += st::msgServiceNameFont->height;
	}
	if (!_caption.isEmpty()) {
		int32 textw = w - st::mediaPadding.left() - st::mediaPadding.right();
		if (!parent->out()) { // substract Download / Save As button
			textw -= st::mediaSaveDelta + _buttonWidth;
		}
		_height += st::webPagePhotoSkip + _caption.countHeight(textw);
	}
	return _height;
}

ImagePtr HistoryVideo::replyPreview() {
	if (data->replyPreview->isNull() && !data->thumb->isNull()) {
		if (data->thumb->loaded()) {
			int w = data->thumb->width(), h = data->thumb->height();
			if (w <= 0) w = 1;
			if (h <= 0) h = 1;
			data->replyPreview = ImagePtr(w > h ? data->thumb->pix(w * st::msgReplyBarSize.height() / h, st::msgReplyBarSize.height()) : data->thumb->pix(st::msgReplyBarSize.height()), "PNG");
		} else {
			data->thumb->load();
		}
	}
	return data->replyPreview;
}

HistoryAudio::HistoryAudio(const MTPDaudio &audio) : HistoryMedia()
, data(App::feedAudio(audio))
, _openl(new AudioOpenLink(data))
, _savel(new AudioSaveLink(data))
, _cancell(new AudioCancelLink(data))
, _dldDone(0)
, _uplDone(0)
{
	_size = formatDurationAndSizeText(data->duration, data->size);

	if (!_openWithWidth) {
		_downloadWidth = st::mediaSaveButton.font->m.width(lang(lng_media_download));
		_openWithWidth = st::mediaSaveButton.font->m.width(lang(lng_media_open_with));
		_cancelWidth = st::mediaSaveButton.font->m.width(lang(lng_media_cancel));
		_buttonWidth = (st::mediaSaveButton.width > 0) ? st::mediaSaveButton.width : ((_downloadWidth > _openWithWidth ? (_downloadWidth > _cancelWidth ? _downloadWidth : _cancelWidth) : _openWithWidth) - st::mediaSaveButton.width);
	}
}

void HistoryAudio::initDimensions(const HistoryItem *parent) {
	_maxw = st::mediaMaxWidth;
	int32 tleft = st::mediaPadding.left() + st::mediaThumbSize + st::mediaPadding.right();
	if (!parent->out()) { // add Download / Save As button
		_maxw += st::mediaSaveDelta + _buttonWidth;
	}

	_minh = st::mediaPadding.top() + st::mediaThumbSize + st::mediaPadding.bottom();
	if (!parent->out() && parent->history()->peer->chat) {
		_minh += st::msgPadding.top() + st::msgNameFont->height;
	}
	if (const HistoryReply *reply = toHistoryReply(parent)) {
		_minh += st::msgReplyPadding.top() + st::msgReplyBarSize.height() + st::msgReplyPadding.bottom();
	} else if (const HistoryForwarded *fwd = toHistoryForwarded(parent)) {
		if (parent->out() || !parent->history()->peer->chat) {
			_minh += st::msgPadding.top();
		}
		_minh += st::msgServiceNameFont->height;
	}
	_height = _minh;
}

void HistoryAudio::draw(QPainter &p, const HistoryItem *parent, bool selected, int32 width) const {
	if (width < 0) width = w;
	if (width < 1) return;

	const HistoryReply *reply = toHistoryReply(parent);
	const HistoryForwarded *fwd = reply ? 0 : toHistoryForwarded(parent);
	int skipy = 0, replyFrom = 0, fwdFrom = 0;
	if (reply) {
		skipy = st::msgReplyPadding.top() + st::msgReplyBarSize.height() + st::msgReplyPadding.bottom();
	} else if (fwd) {
		skipy = st::msgServiceNameFont->height;
	}
	if (!parent->out() && parent->history()->peer->chat) {
		replyFrom = st::msgPadding.top() + st::msgNameFont->height;
		fwdFrom = st::msgPadding.top() + st::msgNameFont->height;
		skipy += replyFrom;
	} else if (fwd) {
		fwdFrom = st::msgPadding.top();
		skipy += fwdFrom;
	}

	bool out = parent->out(), hovered, pressed, already = !data->already().isEmpty(), hasdata = !data->data.isEmpty();
	if (width >= _maxw) {
		width = _maxw;
	}

	if (!data->loader && data->status != FileFailed && !already && !hasdata && data->size < AudioVoiceMsgInMemory) {
		data->save(QString());
	}

	if (!out) { // draw Download / Save As button
		hovered = ((data->loader ? _cancell : _savel) == textlnkOver());
		pressed = hovered && ((data->loader ? _cancell : _savel) == textlnkDown());
		if (hovered && !pressed && textlnkDown()) hovered = false;

		int32 btnw = _buttonWidth, btnh = st::mediaSaveButton.height, btnx = width - _buttonWidth, btny = skipy + (_height - skipy - btnh) / 2;
		style::color bg(selected ? st::msgInSelectBg : (hovered ? st::mediaSaveButton.overBgColor : st::mediaSaveButton.bgColor));
		style::color sh(selected ? st::msgInSelectShadow : st::msgInShadow);
		RoundCorners cors(selected ? MessageInSelectedCorners : (hovered ? ButtonHoverCorners : MessageInCorners));
		App::roundRect(p, btnx, btny, btnw, btnh, bg, cors, &sh);

		p.setPen((hovered ? st::mediaSaveButton.overColor : st::mediaSaveButton.color)->p);
		p.setFont(st::mediaSaveButton.font->f);
		QString btnText(lang(data->loader ? lng_media_cancel : (already ? lng_media_open_with : lng_media_download)));
		int32 btnTextWidth = data->loader ? _cancelWidth : (already ? _openWithWidth : _downloadWidth);
		p.drawText(btnx + (btnw - btnTextWidth) / 2, btny + (pressed ? st::mediaSaveButton.downTextTop : st::mediaSaveButton.textTop) + st::mediaSaveButton.font->ascent, btnText);
		width -= btnw + st::mediaSaveDelta;
	}

	style::color bg(selected ? (out ? st::msgOutSelectBg : st::msgInSelectBg) : (out ? st::msgOutBg : st::msgInBg));
	style::color sh(selected ? (out ? st::msgOutSelectShadow : st::msgInSelectShadow) : (out ? st::msgOutShadow : st::msgInShadow));
	RoundCorners cors(selected ? (out ? MessageOutSelectedCorners : MessageInSelectedCorners) : (out ? MessageOutCorners : MessageInCorners));
	App::roundRect(p, 0, 0, width, _height, bg, cors, &sh);

	if (!parent->out() && parent->history()->peer->chat) {
		p.setFont(st::msgNameFont->f);
		p.setPen(parent->from()->color->p);
		parent->from()->nameText.drawElided(p, st::mediaPadding.left(), st::msgPadding.top(), width - st::mediaPadding.left() - st::mediaPadding.right());
	}
	if (reply) {
		reply->drawReplyTo(p, st::msgReplyPadding.left(), replyFrom, width - st::msgReplyPadding.left() - st::msgReplyPadding.right(), selected);
	} else if (fwd) {
		fwd->drawForwardedFrom(p, st::mediaPadding.left(), fwdFrom, width - st::mediaPadding.left() - st::mediaPadding.right(), selected);
	}

	AudioMsgId playing;
	AudioPlayerState playingState = AudioPlayerStopped;
	int64 playingPosition = 0, playingDuration = 0;
	int32 playingFrequency = 0;
	if (audioPlayer()) {
		audioPlayer()->currentState(&playing, &playingState, &playingPosition, &playingDuration, &playingFrequency);
	}

	QRect img;
	QString statusText;
	if (data->status == FileFailed) {
		statusText = lang(lng_attach_failed);
		img = out ? st::mediaAudioOutImg : st::mediaAudioInImg;
	} else if (data->status == FileUploading) {
		if (_uplTextCache.isEmpty() || _uplDone != data->uploadOffset) {
			_uplDone = data->uploadOffset;
			_uplTextCache = formatDownloadText(_uplDone, data->size);
		}
		statusText = _uplTextCache;
		img = out ? st::mediaAudioOutImg : st::mediaAudioInImg;
	} else if (already || hasdata) {
		bool showPause = false;
		if (playing.msgId == parent->id && !(playingState & AudioPlayerStoppedMask) && playingState != AudioPlayerFinishing) {
			statusText = formatDurationText(playingPosition / (playingFrequency ? playingFrequency : AudioVoiceMsgFrequency)) + qsl(" / ") + formatDurationText(playingDuration / (playingFrequency ? playingFrequency : AudioVoiceMsgFrequency));
			showPause = (playingState == AudioPlayerPlaying || playingState == AudioPlayerResuming || playingState == AudioPlayerStarting);
		} else {
			statusText = formatDurationText(data->duration);
		}
		img = out ? (showPause ? st::mediaPauseOutImg : st::mediaPlayOutImg) : (showPause ? st::mediaPauseInImg : st::mediaPlayInImg);
	} else {
		if (data->loader) {
			int32 offset = data->loader->currentOffset();
			if (_dldTextCache.isEmpty() || _dldDone != offset) {
				_dldDone = offset;
				_dldTextCache = formatDownloadText(_dldDone, data->size);
			}
			statusText = _dldTextCache;
		} else {
			statusText = _size;
		}
		img = out ? st::mediaAudioOutImg : st::mediaAudioInImg;
	}

	p.drawPixmap(QPoint(st::mediaPadding.left(), skipy + st::mediaPadding.top()), App::sprite(), img);
	if (selected) {
		App::roundRect(p, st::mediaPadding.left(), skipy + st::mediaPadding.top(), st::mediaThumbSize, st::mediaThumbSize, textstyleCurrent()->selectOverlay, SelectedOverlayCorners);
	}

	int32 tleft = st::mediaPadding.left() + st::mediaThumbSize + st::mediaPadding.right();
	int32 twidth = width - tleft - st::mediaPadding.right();
	int32 fullTimeWidth = parent->timeWidth(true) + st::msgPadding.right();
	int32 secondwidth = width - tleft - fullTimeWidth;

	p.setFont(st::mediaFont->f);
	p.setPen(st::black->c);
	p.drawText(tleft, skipy + st::mediaPadding.top() + st::mediaNameTop + st::mediaFont->ascent, lang(lng_media_audio));

	style::color status(selected ? (out ? st::mediaOutSelectColor : st::mediaInSelectColor) : (out ? st::mediaOutColor : st::mediaInColor));
	p.setPen(status->p);
	int32 texty = skipy + st::mediaPadding.top() + st::mediaThumbSize - st::mediaDetailsShift - st::mediaFont->height;
	p.drawText(tleft, texty + st::mediaFont->ascent, statusText);
	if (parent->isMediaUnread()) {
		int32 w = st::mediaFont->m.width(statusText);
		if (w + st::mediaUnreadSkip + st::mediaUnreadSize <= twidth) {
			p.setRenderHint(QPainter::HighQualityAntialiasing, true);
			p.setPen(Qt::NoPen);
			p.setBrush((out ? (selected ? st::mediaOutUnreadSelectColor : st::mediaOutUnreadColor) : (selected ? st::mediaInUnreadSelectColor : st::mediaInUnreadColor))->b);
			p.drawEllipse(QRect(tleft + w + st::mediaUnreadSkip, texty + ((st::mediaFont->height - st::mediaUnreadSize) / 2), st::mediaUnreadSize, st::mediaUnreadSize));
			p.setRenderHint(QPainter::HighQualityAntialiasing, false);
		}
	}
	p.setFont(st::msgDateFont->f);

	style::color date(selected ? (out ? st::msgOutSelectDateColor : st::msgInSelectDateColor) : (out ? st::msgOutDateColor : st::msgInDateColor));
	p.setPen(date->p);

	p.drawText(width + st::msgDateDelta.x() - fullTimeWidth + st::msgDateSpace, _height - st::msgPadding.bottom() + st::msgDateDelta.y() - st::msgDateFont->descent, parent->time());
	if (out) {
		QPoint iconPos(width + 5 - st::msgPadding.right() - st::msgCheckRect.pxWidth(), _height + 1 - st::msgPadding.bottom() + st::msgDateDelta.y() - st::msgCheckRect.pxHeight());
		const QRect *iconRect;
		if (parent->id > 0) {
			if (parent->unread()) {
				iconRect = &(selected ? st::msgSelectCheckRect : st::msgCheckRect);
			} else {
				iconRect = &(selected ? st::msgSelectDblCheckRect : st::msgDblCheckRect);
			}
		} else {
			iconRect = &st::msgSendingRect;
		}
		p.drawPixmap(iconPos, App::sprite(), *iconRect);
	}
}

void HistoryAudio::regItem(HistoryItem *item) {
	App::regAudioItem(data, item);
}

void HistoryAudio::unregItem(HistoryItem *item) {
	App::unregAudioItem(data, item);
}

void HistoryAudio::updateFrom(const MTPMessageMedia &media) {
	if (media.type() == mtpc_messageMediaAudio) {
		App::feedAudio(media.c_messageMediaAudio().vaudio, data);
		if (!data->data.isEmpty()) {
			Local::writeAudio(mediaKey(mtpToLocationType(mtpc_inputAudioFileLocation), data->dc, data->id), data->data);
		}
	}
}

const QString HistoryAudio::inDialogsText() const {
	return lang(lng_in_dlg_audio);
}

const QString HistoryAudio::inHistoryText() const {
	return qsl("[ ") + lang(lng_in_dlg_audio) + qsl(" ]");
}

bool HistoryAudio::hasPoint(int32 x, int32 y, const HistoryItem *parent, int32 width) const {
	if (width < 0) width = w;
	if (width >= _maxw) {
		width = _maxw;
	}
	return (x >= 0 && y >= 0 && x < width && y < _height);
}

void HistoryAudio::getState(TextLinkPtr &lnk, HistoryCursorState &state, int32 x, int32 y, const HistoryItem *parent, int32 width) const {
	if (width < 0) width = w;
	if (width < 1) return;

	const HistoryReply *reply = toHistoryReply(parent);
	const HistoryForwarded *fwd = reply ? 0 : toHistoryForwarded(parent);
	int skipy = 0, replyFrom = 0, fwdFrom = 0;
	if (reply) {
		skipy = st::msgReplyPadding.top() + st::msgReplyBarSize.height() + st::msgReplyPadding.bottom();
	} else if (fwd) {
		skipy = st::msgServiceNameFont->height;
	}
	if (!parent->out() && parent->history()->peer->chat) {
		replyFrom = st::msgPadding.top() + st::msgNameFont->height;
		fwdFrom = st::msgPadding.top() + st::msgNameFont->height;
		skipy += replyFrom;
	} else if (fwd) {
		fwdFrom = st::msgPadding.top();
		skipy += fwdFrom;
	}

	bool out = parent->out(), hovered, pressed;
	if (width >= _maxw) {
		width = _maxw;
	}

	if (!out) { // draw Download / Save As button
		int32 btnw = _buttonWidth, btnh = st::mediaSaveButton.height, btnx = width - _buttonWidth, btny = skipy + (_height - skipy - btnh) / 2;
		if (x >= btnx && y >= btny && x < btnx + btnw && y < btny + btnh) {
			lnk = data->loader ? _cancell : _savel;
			return;
		}
		width -= btnw + st::mediaSaveDelta;
	}

	if (!parent->out() && parent->history()->peer->chat) {
		if (x >= st::mediaPadding.left() && y >= st::msgPadding.top() && x < width - st::mediaPadding.left() - st::mediaPadding.right() && x < st::mediaPadding.left() + parent->from()->nameText.maxWidth() && y < replyFrom) {
			lnk = parent->from()->lnk;
			return;
		}
	}
	if (reply) {
		if (x >= 0 && y >= replyFrom + st::msgReplyPadding.top() && x < width && y < skipy - st::msgReplyPadding.bottom()) {
			lnk = reply->replyToLink();
			return;
		}
	} else if (fwd) {
		if (y >= fwdFrom && y < skipy) {
			return fwd->getForwardedState(lnk, state, x - st::mediaPadding.left(), width - st::mediaPadding.left() - st::mediaPadding.right());
		}
	}

	if (x >= 0 && y >= skipy && x < width && y < _height && !data->loader && data->access) {
		lnk = _openl;

		int32 dateX = width - st::msgPadding.right() + st::msgDateDelta.x() - parent->timeWidth(true) + st::msgDateSpace;
		int32 dateY = _height - st::msgPadding.bottom() + st::msgDateDelta.y() - st::msgDateFont->height;
		bool inDate = QRect(dateX, dateY, parent->timeWidth(true) - st::msgDateSpace, st::msgDateFont->height).contains(x, y);
		if (inDate) {
			state = HistoryInDateCursorState;
		}

		return;
	}
}

HistoryMedia *HistoryAudio::clone() const {
	return new HistoryAudio(*this);
}

namespace {
	QString documentName(DocumentData *document) {
		SongData *song = document->song();
		if (!song || (song->title.isEmpty() && song->performer.isEmpty())) return document->name;
		if (song->performer.isEmpty()) return song->title;
		return song->performer + QString::fromUtf8(" \xe2\x80\x93 ") + (song->title.isEmpty() ? qsl("Unknown Track") : song->title);
	}
}

HistoryDocument::HistoryDocument(DocumentData *document) : HistoryMedia()
, data(document)
, _openl(new DocumentOpenLink(data))
, _savel(new DocumentSaveLink(data))
, _cancell(new DocumentCancelLink(data))
, _name(documentName(data))
, _dldDone(0)
, _uplDone(0)
{
	_namew = st::mediaFont->m.width(_name.isEmpty() ? qsl("Document") : _name);
	_size = document->song() ? formatDurationAndSizeText(document->song()->duration, data->size) : formatSizeText(data->size);

	_height = _minh = st::mediaPadding.top() + st::mediaThumbSize + st::mediaPadding.bottom();

	if (!_openWithWidth) {
		_downloadWidth = st::mediaSaveButton.font->m.width(lang(lng_media_download));
		_openWithWidth = st::mediaSaveButton.font->m.width(lang(lng_media_open_with));
		_cancelWidth = st::mediaSaveButton.font->m.width(lang(lng_media_cancel));
		_buttonWidth = (st::mediaSaveButton.width > 0) ? st::mediaSaveButton.width : ((_downloadWidth > _openWithWidth ? (_downloadWidth > _cancelWidth ? _downloadWidth : _cancelWidth) : _openWithWidth) - st::mediaSaveButton.width);
	}

	data->thumb->load();

	int32 tw = data->thumb->width(), th = data->thumb->height();
	if (data->thumb->isNull() || !tw || !th) {
		_thumbw = 0;
	} else if (tw > th) {
		_thumbw = (tw * st::mediaThumbSize) / th;
	} else {
		_thumbw = st::mediaThumbSize;
	}
}

void HistoryDocument::initDimensions(const HistoryItem *parent) {
	if (parent == animated.msg) {
		_maxw = animated.w / cIntRetinaFactor();
		_minh = animated.h / cIntRetinaFactor();
	} else {
		_maxw = st::mediaMaxWidth;
		int32 tleft = st::mediaPadding.left() + st::mediaThumbSize + st::mediaPadding.right();
		if (_namew + tleft + st::mediaPadding.right() > _maxw) {
			_maxw = _namew + tleft + st::mediaPadding.right();
		}
		if (!parent->out()) { // add Download / Save As button
			_maxw += st::mediaSaveDelta + _buttonWidth;
		}
		_minh = st::mediaPadding.top() + st::mediaThumbSize + st::mediaPadding.bottom();

		if (!parent->out() && parent->history()->peer->chat) {
			_minh += st::msgPadding.top() + st::msgNameFont->height;
		}
		if (const HistoryReply *reply = toHistoryReply(parent)) {
			_minh += st::msgReplyPadding.top() + st::msgReplyBarSize.height() + st::msgReplyPadding.bottom();
		} else if (const HistoryForwarded *fwd = toHistoryForwarded(parent)) {
			if (!data->song()) {
				if (parent->out() || !parent->history()->peer->chat) {
					_minh += st::msgPadding.top();
				}
				_minh += st::msgServiceNameFont->height;
			}
		}
	}
	_height = _minh;
}

void HistoryDocument::draw(QPainter &p, const HistoryItem *parent, bool selected, int32 width) const {
	if (width < 0) width = w;
	if (width < 1) return;

	bool out = parent->out(), hovered, pressed, already = !data->already().isEmpty(), hasdata = !data->data.isEmpty();
	if (parent == animated.msg) {
		int32 pw = animated.w / cIntRetinaFactor(), ph = animated.h / cIntRetinaFactor();
		if (width < pw) {
			pw = width;
			ph = (pw == w) ? _height : (pw * animated.h / animated.w);
			if (ph < 1) ph = 1;
		}

		App::roundShadow(p, 0, 0, pw, ph, selected ? st::msgInSelectShadow : st::msgInShadow, selected ? InSelectedShadowCorners : InShadowCorners);

		p.drawPixmap(0, 0, animated.current(pw * cIntRetinaFactor(), ph * cIntRetinaFactor(), true));
		if (selected) {
			App::roundRect(p, 0, 0, pw, ph, textstyleCurrent()->selectOverlay, SelectedOverlayCorners);
		}
		return;
	}

	const HistoryReply *reply = toHistoryReply(parent);
	const HistoryForwarded *fwd = (reply || data->song()) ? 0 : toHistoryForwarded(parent);
	int skipy = 0, replyFrom = 0, fwdFrom = 0;
	if (reply) {
		skipy = st::msgReplyPadding.top() + st::msgReplyBarSize.height() + st::msgReplyPadding.bottom();
	} else if (fwd) {
		skipy = st::msgServiceNameFont->height;
	}
	if (!parent->out() && parent->history()->peer->chat) {
		replyFrom = st::msgPadding.top() + st::msgNameFont->height;
		fwdFrom = st::msgPadding.top() + st::msgNameFont->height;
		skipy += replyFrom;
	} else if (fwd) {
		fwdFrom = st::msgPadding.top();
		skipy += fwdFrom;
	}

	if (width >= _maxw) {
		width = _maxw;
	}

	if (!out) { // draw Download / Save As button
		hovered = ((data->loader ? _cancell : _savel) == textlnkOver());
		pressed = hovered && ((data->loader ? _cancell : _savel) == textlnkDown());
		if (hovered && !pressed && textlnkDown()) hovered = false;

		int32 btnw = _buttonWidth, btnh = st::mediaSaveButton.height, btnx = width - _buttonWidth, btny = skipy + (_height - skipy - btnh) / 2;
		style::color bg(selected ? st::msgInSelectBg : (hovered ? st::mediaSaveButton.overBgColor : st::mediaSaveButton.bgColor));
		style::color sh(selected ? st::msgInSelectShadow : st::msgInShadow);
		RoundCorners cors(selected ? MessageInSelectedCorners : (hovered ? ButtonHoverCorners : MessageInCorners));
		App::roundRect(p, btnx, btny, btnw, btnh, bg, cors, &sh);

		p.setPen((hovered ? st::mediaSaveButton.overColor : st::mediaSaveButton.color)->p);
		p.setFont(st::mediaSaveButton.font->f);
		QString btnText(lang(data->loader ? lng_media_cancel : (already ? lng_media_open_with : lng_media_download)));
		int32 btnTextWidth = data->loader ? _cancelWidth : (already ? _openWithWidth : _downloadWidth);
		p.drawText(btnx + (btnw - btnTextWidth) / 2, btny + (pressed ? st::mediaSaveButton.downTextTop : st::mediaSaveButton.textTop) + st::mediaSaveButton.font->ascent, btnText);
		width -= btnw + st::mediaSaveDelta;
	}

	style::color bg(selected ? (out ? st::msgOutSelectBg : st::msgInSelectBg) : (out ? st::msgOutBg : st::msgInBg));
	style::color sh(selected ? (out ? st::msgOutSelectShadow : st::msgInSelectShadow) : (out ? st::msgOutShadow : st::msgInShadow));
	RoundCorners cors(selected ? (out ? MessageOutSelectedCorners : MessageInSelectedCorners) : (out ? MessageOutCorners : MessageInCorners));
	App::roundRect(p, 0, 0, width, _height, bg, cors, &sh);

	if (!parent->out() && parent->history()->peer->chat) {
		p.setFont(st::msgNameFont->f);
		p.setPen(parent->from()->color->p);
		parent->from()->nameText.drawElided(p, st::mediaPadding.left(), st::msgPadding.top(), width - st::mediaPadding.left() - st::mediaPadding.right());
	}
	if (reply) {
		reply->drawReplyTo(p, st::msgReplyPadding.left(), replyFrom, width - st::msgReplyPadding.left() - st::msgReplyPadding.right(), selected);
	} else if (fwd) {
		fwd->drawForwardedFrom(p, st::mediaPadding.left(), fwdFrom, width - st::mediaPadding.left() - st::mediaPadding.right(), selected);
	}

	QString statusText;
	if (data->song()) {
		SongMsgId playing;
		AudioPlayerState playingState = AudioPlayerStopped;
		int64 playingPosition = 0, playingDuration = 0;
		int32 playingFrequency = 0;
		if (audioPlayer()) {
			audioPlayer()->currentState(&playing, &playingState, &playingPosition, &playingDuration, &playingFrequency);
		}

		QRect img;
		if (data->status == FileFailed) {
			statusText = lang(lng_attach_failed);
			img = out ? st::mediaMusicOutImg : st::mediaMusicInImg;
		} else if (data->status == FileUploading) {
			if (_uplTextCache.isEmpty() || _uplDone != data->uploadOffset) {
				_uplDone = data->uploadOffset;
				_uplTextCache = formatDownloadText(_uplDone, data->size);
			}
			statusText = _uplTextCache;
			img = out ? st::mediaMusicOutImg : st::mediaMusicInImg;
		} else if (already || hasdata) {
			bool showPause = false;
			if (playing.msgId == parent->id && !(playingState & AudioPlayerStoppedMask) && playingState != AudioPlayerFinishing) {
				statusText = formatDurationText(playingPosition / (playingFrequency ? playingFrequency : AudioVoiceMsgFrequency)) + qsl(" / ") + formatDurationText(playingDuration / (playingFrequency ? playingFrequency : AudioVoiceMsgFrequency));
				showPause = (playingState == AudioPlayerPlaying || playingState == AudioPlayerResuming || playingState == AudioPlayerStarting);
			} else {
				statusText = formatDurationText(data->song()->duration);
			}
			if (!showPause && playing.msgId == parent->id && App::main() && App::main()->player()->seekingSong(playing)) showPause = true;
			img = out ? (showPause ? st::mediaPauseOutImg : st::mediaPlayOutImg) : (showPause ? st::mediaPauseInImg : st::mediaPlayInImg);
		} else {
			if (data->loader) {
				int32 offset = data->loader->currentOffset();
				if (_dldTextCache.isEmpty() || _dldDone != offset) {
					_dldDone = offset;
					_dldTextCache = formatDownloadText(_dldDone, data->size);
				}
				statusText = _dldTextCache;
			} else {
				statusText = _size;
			}
			img = out ? st::mediaMusicOutImg : st::mediaMusicInImg;
		}

		p.drawPixmap(QPoint(st::mediaPadding.left(), skipy + st::mediaPadding.top()), App::sprite(), img);
	} else {
		if (data->status == FileFailed) {
			statusText = lang(lng_attach_failed);
		} else if (data->status == FileUploading) {
			if (_uplTextCache.isEmpty() || _uplDone != data->uploadOffset) {
				_uplDone = data->uploadOffset;
				_uplTextCache = formatDownloadText(_uplDone, data->size);
			}
			statusText = _uplTextCache;
		} else if (data->loader) {
			int32 offset = data->loader->currentOffset();
			if (_dldTextCache.isEmpty() || _dldDone != offset) {
				_dldDone = offset;
				_dldTextCache = formatDownloadText(_dldDone, data->size);
			}
			statusText = _dldTextCache;
		} else {
			statusText = _size;
		}

		if (_thumbw) {
			data->thumb->checkload();
			p.drawPixmap(QPoint(st::mediaPadding.left(), skipy + st::mediaPadding.top()), data->thumb->pixSingle(_thumbw, 0, st::mediaThumbSize, st::mediaThumbSize));
		} else {
			p.drawPixmap(QPoint(st::mediaPadding.left(), skipy + st::mediaPadding.top()), App::sprite(), (out ? st::mediaDocOutImg : st::mediaDocInImg));
		}
	}
	if (selected) {
		App::roundRect(p, st::mediaPadding.left(), skipy + st::mediaPadding.top(), st::mediaThumbSize, st::mediaThumbSize, textstyleCurrent()->selectOverlay, SelectedOverlayCorners);
	}

	int32 tleft = st::mediaPadding.left() + st::mediaThumbSize + st::mediaPadding.right();
	int32 twidth = width - tleft - st::mediaPadding.right();
	int32 fullTimeWidth = parent->timeWidth(true) + st::msgPadding.right();
	int32 secondwidth = width - tleft - fullTimeWidth;

	p.setFont(st::mediaFont->f);
	p.setPen(st::black->c);
	if (twidth < _namew) {
		p.drawText(tleft, skipy + st::mediaPadding.top() + st::mediaNameTop + st::mediaFont->ascent, st::mediaFont->m.elidedText(_name, Qt::ElideRight, twidth));
	} else {
		p.drawText(tleft, skipy + st::mediaPadding.top() + st::mediaNameTop + st::mediaFont->ascent, _name);
	}

	style::color status(selected ? (out ? st::mediaOutSelectColor : st::mediaInSelectColor) : (out ? st::mediaOutColor : st::mediaInColor));
	p.setPen(status->p);

	p.drawText(tleft, skipy + st::mediaPadding.top() + st::mediaThumbSize - st::mediaDetailsShift - st::mediaFont->descent, statusText);

	p.setFont(st::msgDateFont->f);

	style::color date(selected ? (out ? st::msgOutSelectDateColor : st::msgInSelectDateColor) : (out ? st::msgOutDateColor : st::msgInDateColor));
	p.setPen(date->p);

	p.drawText(width + st::msgDateDelta.x() - fullTimeWidth + st::msgDateSpace, _height - st::msgPadding.bottom() + st::msgDateDelta.y() - st::msgDateFont->descent, parent->time());
	if (out) {
		QPoint iconPos(width + 5 - st::msgPadding.right() - st::msgCheckRect.pxWidth(), _height + 1 - st::msgPadding.bottom() + st::msgDateDelta.y() - st::msgCheckRect.pxHeight());
		const QRect *iconRect;
		if (parent->id > 0) {
			if (parent->unread()) {
				iconRect = &(selected ? st::msgSelectCheckRect : st::msgCheckRect);
			} else {
				iconRect = &(selected ? st::msgSelectDblCheckRect : st::msgDblCheckRect);
			}
		} else {
			iconRect = &st::msgSendingRect;
		}
		p.drawPixmap(iconPos, App::sprite(), *iconRect);
	}
}

void HistoryDocument::drawInPlaylist(QPainter &p, const HistoryItem *parent, bool selected, bool over, int32 width) const {
	bool out = parent->out(), already = !data->already().isEmpty(), hasdata = !data->data.isEmpty();
	int32 height = st::mediaPadding.top() + st::mediaThumbSize + st::mediaPadding.bottom();

	style::color bg(selected ? st::msgInSelectBg : (over ? st::playlistHoverBg : st::msgInBg));
	p.fillRect(0, 0, width, height, bg->b);

	QString statusText;
	if (data->song()) {
		SongMsgId playing;
		AudioPlayerState playingState = AudioPlayerStopped;
		int64 playingPosition = 0, playingDuration = 0;
		int32 playingFrequency = 0;
		if (audioPlayer()) {
			audioPlayer()->currentState(&playing, &playingState, &playingPosition, &playingDuration, &playingFrequency);
		}

		QRect img;
		if (data->status == FileFailed) {
			statusText = lang(lng_attach_failed);
			img = st::mediaMusicInImg;
		} else if (data->status == FileUploading) {
			if (_uplTextCache.isEmpty() || _uplDone != data->uploadOffset) {
				_uplDone = data->uploadOffset;
				_uplTextCache = formatDownloadText(_uplDone, data->size);
			}
			statusText = _uplTextCache;
			img = st::mediaMusicInImg;
		} else if (already || hasdata) {
			bool isPlaying = (playing.msgId == parent->id);
			bool showPause = false;
			if (playing.msgId == parent->id && !(playingState & AudioPlayerStoppedMask) && playingState != AudioPlayerFinishing) {
				statusText = formatDurationText(playingPosition / (playingFrequency ? playingFrequency : AudioVoiceMsgFrequency)) + qsl(" / ") + formatDurationText(playingDuration / (playingFrequency ? playingFrequency : AudioVoiceMsgFrequency));
				showPause = (playingState == AudioPlayerPlaying || playingState == AudioPlayerResuming || playingState == AudioPlayerStarting);
			} else {
				statusText = formatDurationText(data->song()->duration);
			}
			if (!showPause && playing.msgId == parent->id && App::main() && App::main()->player()->seekingSong(playing)) showPause = true;
			img = isPlaying ? (showPause ? st::mediaPauseOutImg : st::mediaPlayOutImg) : (showPause ? st::mediaPauseInImg : st::mediaPlayInImg);
		} else {
			if (data->loader) {
				int32 offset = data->loader->currentOffset();
				if (_dldTextCache.isEmpty() || _dldDone != offset) {
					_dldDone = offset;
					_dldTextCache = formatDownloadText(_dldDone, data->size);
				}
				statusText = _dldTextCache;
			} else {
				statusText = _size;
			}
			img = st::mediaMusicInImg;
		}

		p.drawPixmap(QPoint(st::mediaPadding.left(), st::mediaPadding.top()), App::sprite(), img);
	} else {
		if (data->status == FileFailed) {
			statusText = lang(lng_attach_failed);
		} else if (data->status == FileUploading) {
			if (_uplTextCache.isEmpty() || _uplDone != data->uploadOffset) {
				_uplDone = data->uploadOffset;
				_uplTextCache = formatDownloadText(_uplDone, data->size);
			}
			statusText = _uplTextCache;
		} else if (data->loader) {
			int32 offset = data->loader->currentOffset();
			if (_dldTextCache.isEmpty() || _dldDone != offset) {
				_dldDone = offset;
				_dldTextCache = formatDownloadText(_dldDone, data->size);
			}
			statusText = _dldTextCache;
		} else {
			statusText = _size;
		}

		if (_thumbw) {
			data->thumb->checkload();
			p.drawPixmap(QPoint(st::mediaPadding.left(), st::mediaPadding.top()), data->thumb->pixSingle(_thumbw, 0, st::mediaThumbSize, st::mediaThumbSize));
		} else {
			p.drawPixmap(QPoint(st::mediaPadding.left(), st::mediaPadding.top()), App::sprite(), st::mediaDocInImg);
		}
	}
	if (selected) {
		App::roundRect(p, st::mediaPadding.left(), st::mediaPadding.top(), st::mediaThumbSize, st::mediaThumbSize, textstyleCurrent()->selectOverlay, SelectedOverlayCorners);
	}

	int32 tleft = st::mediaPadding.left() + st::mediaThumbSize + st::mediaPadding.right();
	int32 twidth = width - tleft - st::mediaPadding.right();
	int32 fullTimeWidth = parent->timeWidth(true) + st::msgPadding.right();
	int32 secondwidth = width - tleft - fullTimeWidth;

	p.setFont(st::mediaFont->f);
	p.setPen(st::black->c);
	if (twidth < _namew) {
		p.drawText(tleft, st::mediaPadding.top() + st::mediaNameTop + st::mediaFont->ascent, st::mediaFont->m.elidedText(_name, Qt::ElideRight, twidth));
	} else {
		p.drawText(tleft, st::mediaPadding.top() + st::mediaNameTop + st::mediaFont->ascent, _name);
	}

	style::color status(selected ? st::mediaInSelectColor : st::mediaInColor);
	p.setPen(status->p);
	p.drawText(tleft, st::mediaPadding.top() + st::mediaThumbSize - st::mediaDetailsShift - st::mediaFont->descent, statusText);
}

TextLinkPtr HistoryDocument::linkInPlaylist() {
	if (!data->loader && data->access) {
		return _openl;
	}
	return TextLinkPtr();
}

void HistoryDocument::regItem(HistoryItem *item) {
	App::regDocumentItem(data, item);
}

void HistoryDocument::unregItem(HistoryItem *item) {
	App::unregDocumentItem(data, item);
}

void HistoryDocument::updateFrom(const MTPMessageMedia &media) {
	if (media.type() == mtpc_messageMediaDocument) {
		App::feedDocument(media.c_messageMediaDocument().vdocument, data);
	}
}

int32 HistoryDocument::resize(int32 width, bool dontRecountText, const HistoryItem *parent) {
	w = qMin(width, _maxw);
	if (parent == animated.msg) {
		if (w > st::maxMediaSize) {
			w = st::maxMediaSize;
		}
		_height = animated.h / cIntRetinaFactor();
		if (animated.w / cIntRetinaFactor() > w) {
			_height = (w * _height / (animated.w / cIntRetinaFactor()));
			if (_height <= 0) _height = 1;
		}
	} else {
		_height = _minh;
	}
	return _height;
}

const QString HistoryDocument::inDialogsText() const {
	return _name.isEmpty() ? lang(lng_in_dlg_file) : _name;
}

const QString HistoryDocument::inHistoryText() const {
	return qsl("[ ") + lang(lng_in_dlg_file) + (_name.isEmpty() ? QString() : (qsl(" : ") + _name)) + qsl(" ]");
}

bool HistoryDocument::hasPoint(int32 x, int32 y, const HistoryItem *parent, int32 width) const {
	if (width < 0) width = w;
	if (width >= _maxw) {
		width = _maxw;
	}
	if (parent == animated.msg) {
		int32 h = (width == w) ? _height : (width * animated.h / animated.w);
		if (h < 1) h = 1;
		return (x >= 0 && y >= 0 && x < width && y < h);
	}
	return (x >= 0 && y >= 0 && x < width && y < _height);
}

int32 HistoryDocument::countHeight(const HistoryItem *parent, int32 width) const {
	if (width < 0) width = w;
	if (width >= _maxw) {
		width = _maxw;
	}
	if (parent == animated.msg) {
		int32 h = (width == w) ? _height : (width * animated.h / animated.w);
		if (h < 1) h = 1;
		return h;
	}
	return _height;
}

void HistoryDocument::getState(TextLinkPtr &lnk, HistoryCursorState &state, int32 x, int32 y, const HistoryItem *parent, int32 width) const {
	if (width < 0) width = w;
	if (width < 1) return;

	bool out = parent->out(), hovered, pressed;
	if (width >= _maxw) {
		width = _maxw;
	}
	if (parent == animated.msg) {
		int32 h = (width == w) ? _height : (width * animated.h / animated.w);
		if (h < 1) h = 1;
		lnk = (x >= 0 && y >= 0 && x < width && y < h) ? _openl : TextLinkPtr();
		return;
	}

	const HistoryReply *reply = toHistoryReply(parent);
	const HistoryForwarded *fwd = (reply || data->song()) ? 0 : toHistoryForwarded(parent);
	int skipy = 0, replyFrom = 0, fwdFrom = 0;
	if (reply) {
		skipy = st::msgReplyPadding.top() + st::msgReplyBarSize.height() + st::msgReplyPadding.bottom();
	} else if (fwd) {
		skipy = st::msgServiceNameFont->height;
	}
	if (!parent->out() && parent->history()->peer->chat) {
		replyFrom = st::msgPadding.top() + st::msgNameFont->height;
		fwdFrom = st::msgPadding.top() + st::msgNameFont->height;
		skipy += replyFrom;
	} else if (fwd) {
		fwdFrom = st::msgPadding.top();
		skipy += fwdFrom;
	}

	if (!out) { // draw Download / Save As button
		int32 btnw = _buttonWidth, btnh = st::mediaSaveButton.height, btnx = width - _buttonWidth, btny = skipy + (_height - skipy - btnh) / 2;
		if (x >= btnx && y >= btny && x < btnx + btnw && y < btny + btnh) {
			lnk = data->loader ? _cancell : _savel;
			return;
		}
		width -= btnw + st::mediaSaveDelta;
	}

	if (!parent->out() && parent->history()->peer->chat) {
		if (x >= st::mediaPadding.left() && y >= st::msgPadding.top() && x < width - st::mediaPadding.left() - st::mediaPadding.right() && x < st::mediaPadding.left() + parent->from()->nameText.maxWidth() && y < replyFrom) {
			lnk = parent->from()->lnk;
			return;
		}
	}
	if (reply) {
		if (x >= 0 && y >= replyFrom + st::msgReplyPadding.top() && x < width && y < skipy - st::msgReplyPadding.bottom()) {
			lnk = reply->replyToLink();
			return;
		}
	} else if (fwd) {
		if (y >= fwdFrom && y < skipy) {
			return fwd->getForwardedState(lnk, state, x - st::mediaPadding.left(), width - st::mediaPadding.left() - st::mediaPadding.right());
		}
	}

	if (x >= 0 && y >= skipy && x < width && y < _height && !data->loader && data->access) {
		lnk = _openl;

		int32 dateX = width - st::msgPadding.right() + st::msgDateDelta.x() - parent->timeWidth(true) + st::msgDateSpace;
		int32 dateY = _height - st::msgPadding.bottom() + st::msgDateDelta.y() - st::msgDateFont->height;
		bool inDate = QRect(dateX, dateY, parent->timeWidth(true) - st::msgDateSpace, st::msgDateFont->height).contains(x, y);
		if (inDate) {
			state = HistoryInDateCursorState;
		}

		return;
	}
}

HistoryMedia *HistoryDocument::clone() const {
	return new HistoryDocument(*this);
}

ImagePtr HistoryDocument::replyPreview() {
	return data->makeReplyPreview();
}

HistorySticker::HistorySticker(DocumentData *document) : HistoryMedia()
, pixw(1), pixh(1), data(document), lastw(0)
{
	data->thumb->load();
	if (!data->sticker()->alt.isEmpty()) {
		_emoji = data->sticker()->alt;
	}
}

void HistorySticker::initDimensions(const HistoryItem *parent) {
	pixw = data->dimensions.width();
	pixh = data->dimensions.height();
	if (pixw > st::maxStickerSize) {
		pixh = (st::maxStickerSize * pixh) / pixw;
		pixw = st::maxStickerSize;
	}
	if (pixh > st::maxStickerSize) {
		pixw = (st::maxStickerSize * pixw) / pixh;
		pixh = st::maxStickerSize;
	}
	if (pixw < 1) pixw = 1;
	if (pixh < 1) pixh = 1;
	_maxw = qMax(pixw, int16(st::minPhotoSize));
	_minh = qMax(pixh, int16(st::minPhotoSize));
	if (const HistoryReply *reply = toHistoryReply(parent)) {
		_maxw += st::msgReplyPadding.left() + reply->replyToWidth();
	}
	_height = _minh;
	w = qMin(lastw, _maxw);
}

void HistorySticker::draw(QPainter &p, const HistoryItem *parent, bool selected, int32 width) const {
	if (width < 0) width = w;
	if (width < 1) return;
	if (width > _maxw) width = _maxw;

	int32 usew = _maxw, usex = 0;
	const HistoryReply *reply = toHistoryReply(parent);
	if (reply) {
		usew -= st::msgReplyPadding.left() + reply->replyToWidth();
		if (parent->out()) {
			usex = width - usew;
		}
	}

	bool out = parent->out(), hovered, pressed, already = !data->already().isEmpty(), hasdata = !data->data.isEmpty();
	if (!data->loader && data->status != FileFailed && !already && !hasdata) {
		data->save(QString());
	}
	if (data->sticker()->img->isNull() && (already || hasdata)) {
		if (already) {
			data->sticker()->img = ImagePtr(data->already());
		} else {
			data->sticker()->img = ImagePtr(data->data);
		}
	}
	if (selected) {
		if (data->sticker()->img->isNull()) {
			p.drawPixmap(QPoint(usex + (usew - pixw) / 2, (_minh - pixh) / 2), data->thumb->pixBlurredColored(st::msgStickerOverlay, pixw, pixh));
		} else {
			p.drawPixmap(QPoint(usex + (usew - pixw) / 2, (_minh - pixh) / 2), data->sticker()->img->pixColored(st::msgStickerOverlay, pixw, pixh));
		}
	} else {
		if (data->sticker()->img->isNull()) {
			p.drawPixmap(QPoint(usex + (usew - pixw) / 2, (_minh - pixh) / 2), data->thumb->pixBlurred(pixw, pixh));
		} else {
			p.drawPixmap(QPoint(usex + (usew - pixw) / 2, (_minh - pixh) / 2), data->sticker()->img->pix(pixw, pixh));
		}
	}

	// date
	QString time(parent->time());
	if (time.isEmpty()) return;
	int32 dateX = usex + usew - parent->timeWidth(false) - st::msgDateImgDelta - 2 * st::msgDateImgPadding.x();
	int32 dateY = _height - st::msgDateFont->height - 2 * st::msgDateImgPadding.y() - st::msgDateImgDelta;
	if (parent->out()) {
		dateX -= st::msgCheckRect.pxWidth() + st::msgDateImgCheckSpace;
	}
	int32 dateW = usex + usew - dateX - st::msgDateImgDelta;
	int32 dateH = _height - dateY - st::msgDateImgDelta;

	App::roundRect(p, dateX, dateY, dateW, dateH, selected ? st::msgDateImgSelectBg : st::msgDateImgBg, selected ? DateSelectedCorners : DateCorners);

	p.setFont(st::msgDateFont->f);
	p.setPen(st::msgDateImgColor->p);
	p.drawText(dateX + st::msgDateImgPadding.x(), dateY + st::msgDateImgPadding.y() + st::msgDateFont->ascent, time);
	if (out) {
		QPoint iconPos(dateX - 2 + dateW - st::msgDateImgCheckSpace - st::msgCheckRect.pxWidth(), dateY + (dateH - st::msgCheckRect.pxHeight()) / 2);
		const QRect *iconRect;
		if (parent->id > 0) {
			if (parent->unread()) {
				iconRect = &st::msgImgCheckRect;
			} else {
				iconRect = &st::msgImgDblCheckRect;
			}
		} else {
			iconRect = &st::msgImgSendingRect;
		}
		p.drawPixmap(iconPos, App::sprite(), *iconRect);
	}

	if (reply) {
		int32 rw = width - usew - st::msgReplyPadding.left(), rh = st::msgReplyPadding.top() + st::msgReplyBarSize.height() + st::msgReplyPadding.bottom();
		int32 rx = parent->out() ? 0 : (usew + st::msgReplyPadding.left()), ry = _height - rh;
		
		App::roundRect(p, rx, ry, rw, rh, selected ? App::msgServiceSelectBg() : App::msgServiceBg(), selected ? ServiceSelectedCorners : ServiceCorners);

		reply->drawReplyTo(p, rx + st::msgReplyPadding.left(), ry, rw - st::msgReplyPadding.left() - st::msgReplyPadding.right(), selected, true);
	}
}

int32 HistorySticker::resize(int32 width, bool dontRecountText, const HistoryItem *parent) {
	w = qMin(width, _maxw);
	lastw = width;
	return _height;
}

void HistorySticker::regItem(HistoryItem *item) {
	App::regDocumentItem(data, item);
}

void HistorySticker::unregItem(HistoryItem *item) {
	App::unregDocumentItem(data, item);
}

void HistorySticker::updateFrom(const MTPMessageMedia &media) {
	if (media.type() == mtpc_messageMediaDocument) {
		App::feedDocument(media.c_messageMediaDocument().vdocument, data);
		if (!data->data.isEmpty()) {
			Local::writeStickerImage(mediaKey(mtpToLocationType(mtpc_inputDocumentFileLocation), data->dc, data->id), data->data);
		}
	}
}

const QString HistorySticker::inDialogsText() const {
	return _emoji.isEmpty() ? lang(lng_in_dlg_sticker) : lng_in_dlg_sticker_emoji(lt_emoji, _emoji);
}

const QString HistorySticker::inHistoryText() const {
	return qsl("[ ") + inDialogsText() + qsl(" ]");
}

bool HistorySticker::hasPoint(int32 x, int32 y, const HistoryItem *parent, int32 width) const {
	return (x >= 0 && y >= 0 && x < _maxw && y < _minh);
}

int32 HistorySticker::countHeight(const HistoryItem *parent, int32 width) const {
	return _minh;
}

void HistorySticker::getState(TextLinkPtr &lnk, HistoryCursorState &state, int32 x, int32 y, const HistoryItem *parent, int32 width) const {
	if (width < 0) width = w;
	if (width < 1) return;

	if (width > _maxw) width = _maxw;

	int32 usew = _maxw, usex = 0;
	const HistoryReply *reply = toHistoryReply(parent);
	if (reply) {
		usew -= reply->replyToWidth();
		if (parent->out()) {
			usex = width - usew;
		}

		int32 rw = width - usew, rh = st::msgReplyPadding.top() + st::msgReplyBarSize.height() + st::msgReplyPadding.bottom();
		int32 rx = parent->out() ? 0 : usew, ry = _height - rh;
		if (x >= rx && y >= ry && x < rx + rw && y < ry + rh) {
			lnk = reply->replyToLink();
			return;
		}
	}
	int32 dateX = usex + usew - parent->timeWidth(false) - st::msgDateImgDelta - st::msgDateImgPadding.x();
	int32 dateY = _height - st::msgDateFont->height - st::msgDateImgDelta - st::msgDateImgPadding.y();
	if (parent->out()) {
		dateX -= st::msgCheckRect.pxWidth() + st::msgDateImgCheckSpace;
	}
	bool inDate = QRect(dateX, dateY, parent->timeWidth(true) - st::msgDateSpace, st::msgDateFont->height).contains(x, y);
	if (inDate) {
		state = HistoryInDateCursorState;
	}
}

HistoryMedia *HistorySticker::clone() const {
	return new HistorySticker(*this);
}

HistoryContact::HistoryContact(int32 userId, const QString &first, const QString &last, const QString &phone) : HistoryMedia(0)
, userId(userId)
, phone(App::formatPhone(phone))
, contact(App::userLoaded(userId))
{
	_maxw = st::mediaMaxWidth;
	name.setText(st::mediaFont, (first + ' ' + last).trimmed(), _textNameOptions);

	phonew = st::mediaFont->m.width(phone);

	if (contact) {
		if (contact->phone.isEmpty()) {
			contact->setPhone(phone);
		}
		if (contact->contact < 0) {
			contact->contact = 0;
		}
		contact->photo->load();
	}
}

void HistoryContact::initDimensions(const HistoryItem *parent) {
	int32 tleft = st::mediaPadding.left() + st::mediaThumbSize + st::mediaPadding.right();
	int32 fullTimeWidth = parent->timeWidth(true) + st::msgPadding.right();
	if (name.maxWidth() + tleft + fullTimeWidth > _maxw) {
		_maxw = name.maxWidth() + tleft + fullTimeWidth;
	}
	if (phonew + tleft + st::mediaPadding.right() > _maxw) {
		_maxw = phonew + tleft + st::mediaPadding.right();
	}
	_minh = st::mediaPadding.top() + st::mediaThumbSize + st::mediaPadding.bottom();
	if (!parent->out() && parent->history()->peer->chat) {
		_minh += st::msgPadding.top() + st::msgNameFont->height;
	}
	if (const HistoryReply *reply = toHistoryReply(parent)) {
		_minh += st::msgReplyPadding.top() + st::msgReplyBarSize.height() + st::msgReplyPadding.bottom();
	} else if (const HistoryForwarded *fwd = toHistoryForwarded(parent)) {
		if (parent->out() || !parent->history()->peer->chat) {
			_minh += st::msgPadding.top();
		}
		_minh += st::msgServiceNameFont->height;
	}
	_height = _minh;
}

const QString HistoryContact::inDialogsText() const {
	return lang(lng_in_dlg_contact);
}

const QString HistoryContact::inHistoryText() const {
	return qsl("[ ") + lang(lng_in_dlg_contact) + qsl(" : ") + name.original(0, 0xFFFF, false) + qsl(", ") + phone + qsl(" ]");
}

bool HistoryContact::hasPoint(int32 x, int32 y, const HistoryItem *parent, int32 width) const {
	if (width < 0) width = w;
	return (x >= 0 && y <= 0 && x < w && y < _height);
}

void HistoryContact::getState(TextLinkPtr &lnk, HistoryCursorState &state, int32 x, int32 y, const HistoryItem *parent, int32 width) const {
	if (width < 0) width = w;

	const HistoryReply *reply = toHistoryReply(parent);
	const HistoryForwarded *fwd = reply ? 0 : toHistoryForwarded(parent);
	int skipy = 0, replyFrom = 0, fwdFrom = 0;
	if (reply) {
		skipy = st::msgReplyPadding.top() + st::msgReplyBarSize.height() + st::msgReplyPadding.bottom();
	} else if (fwd) {
		skipy = st::msgServiceNameFont->height;
	}
	if (!parent->out() && parent->history()->peer->chat) {
		replyFrom = st::msgPadding.top() + st::msgNameFont->height;
		fwdFrom = st::msgPadding.top() + st::msgNameFont->height;
		skipy += replyFrom;
	} else if (fwd) {
		fwdFrom = st::msgPadding.top();
		skipy += fwdFrom;
	}

	if (!parent->out() && parent->history()->peer->chat) {
		if (x >= st::mediaPadding.left() && y >= st::msgPadding.top() && x < width - st::mediaPadding.left() - st::mediaPadding.right() && x < st::mediaPadding.left() + parent->from()->nameText.maxWidth() && y < replyFrom) {
			lnk = parent->from()->lnk;
			return;
		}
	}
	if (reply) {
		if (x >= 0 && y >= replyFrom + st::msgReplyPadding.top() && x < width && y < skipy - st::msgReplyPadding.bottom()) {
			lnk = reply->replyToLink();
			return;
		}
	} else if (fwd) {
		if (y >= fwdFrom && y < skipy) {
			return fwd->getForwardedState(lnk, state, x - st::mediaPadding.left(), width - st::mediaPadding.left() - st::mediaPadding.right());
		}
	}

	if (x >= 0 && y >= skipy && x < w && y < _height && contact) {
		lnk = contact->lnk;
		return;
	}
}

HistoryMedia *HistoryContact::clone() const {
	QStringList names = name.original(0, 0xFFFF, false).split(QChar(' '), QString::SkipEmptyParts);
	if (names.isEmpty()) {
		names.push_back(QString());
	}
	QString fname = names.front();
	names.pop_front();
	HistoryContact *result = new HistoryContact(userId, fname, names.join(QChar(' ')), phone);
	return result;
}

void HistoryContact::draw(QPainter &p, const HistoryItem *parent, bool selected, int32 width) const {
	if (width < 0) width = w;
	if (width < 1) return;

	const HistoryReply *reply = toHistoryReply(parent);
	const HistoryForwarded *fwd = reply ? 0 : toHistoryForwarded(parent);
	int skipy = 0, replyFrom = 0, fwdFrom = 0;
	if (reply) {
		skipy = st::msgReplyPadding.top() + st::msgReplyBarSize.height() + st::msgReplyPadding.bottom();
	} else if (fwd) {
		skipy = st::msgServiceNameFont->height;
	}
	if (!parent->out() && parent->history()->peer->chat) {
		replyFrom = st::msgPadding.top() + st::msgNameFont->height;
		fwdFrom = st::msgPadding.top() + st::msgNameFont->height;
		skipy += replyFrom;
	} else if (fwd) {
		fwdFrom = st::msgPadding.top();
		skipy += fwdFrom;
	}

	bool out = parent->out();
	if (width >= _maxw) {
		width = _maxw;
	}

	style::color bg(selected ? (out ? st::msgOutSelectBg : st::msgInSelectBg) : (out ? st::msgOutBg : st::msgInBg));
	style::color sh(selected ? (out ? st::msgOutSelectShadow : st::msgInSelectShadow) : (out ? st::msgOutShadow : st::msgInShadow));
	RoundCorners cors(selected ? (out ? MessageOutSelectedCorners : MessageInSelectedCorners) : (out ? MessageOutCorners : MessageInCorners));
	App::roundRect(p, 0, 0, width, _height, bg, cors, &sh);

	if (!parent->out() && parent->history()->peer->chat) {
		p.setFont(st::msgNameFont->f);
		p.setPen(parent->from()->color->p);
		parent->from()->nameText.drawElided(p, st::mediaPadding.left(), st::msgPadding.top(), width - st::mediaPadding.left() - st::mediaPadding.right());
	}
	if (reply) {
		reply->drawReplyTo(p, st::msgReplyPadding.left(), replyFrom, width - st::msgReplyPadding.left() - st::msgReplyPadding.right(), selected);
	} else if (fwd) {
		fwd->drawForwardedFrom(p, st::mediaPadding.left(), fwdFrom, width - st::mediaPadding.left() - st::mediaPadding.right(), selected);
	}

	p.drawPixmap(st::mediaPadding.left(), skipy + st::mediaPadding.top(), (contact ? contact->photo : userDefPhoto(1))->pixRounded(st::mediaThumbSize));
	if (selected) {
		App::roundRect(p, st::mediaPadding.left(), skipy + st::mediaPadding.top(), st::mediaThumbSize, st::mediaThumbSize, textstyleCurrent()->selectOverlay, SelectedOverlayCorners);
	}

	int32 tleft = st::mediaPadding.left() + st::mediaThumbSize + st::mediaPadding.right();
	int32 twidth = width - tleft - st::mediaPadding.right();
	int32 fullTimeWidth = parent->timeWidth(true) + st::msgPadding.right();
	int32 secondwidth = width - tleft - fullTimeWidth;

	p.setFont(st::mediaFont->f);
	p.setPen(st::black->c);
	if (twidth < phonew) {
		p.drawText(tleft, skipy + st::mediaPadding.top() + st::mediaNameTop + st::mediaFont->ascent, st::mediaFont->m.elidedText(phone, Qt::ElideRight, twidth));
	} else {
		p.drawText(tleft, skipy + st::mediaPadding.top() + st::mediaNameTop + st::mediaFont->ascent, phone);
	}

	style::color status(selected ? (out ? st::mediaOutSelectColor : st::mediaInSelectColor) : (out ? st::mediaOutColor : st::mediaInColor));
	p.setPen(status->p);

	name.drawElided(p, tleft, skipy + st::mediaPadding.top() + st::mediaThumbSize - st::mediaDetailsShift - st::mediaFont->height, secondwidth);

	p.setFont(st::msgDateFont->f);

	style::color date(selected ? (out ? st::msgOutSelectDateColor : st::msgInSelectDateColor) : (out ? st::msgOutDateColor : st::msgInDateColor));
	p.setPen(date->p);

	p.drawText(width + st::msgDateDelta.x() - fullTimeWidth + st::msgDateSpace, _height - st::msgPadding.bottom() + st::msgDateDelta.y() - st::msgDateFont->descent, parent->time());
	if (out) {
		QPoint iconPos(width + 5 - st::msgPadding.right() - st::msgCheckRect.pxWidth(), _height + 1 - st::msgPadding.bottom() + st::msgDateDelta.y() - st::msgCheckRect.pxHeight());
		const QRect *iconRect;
		if (parent->id > 0) {
			if (parent->unread()) {
				iconRect = &(selected ? st::msgSelectCheckRect : st::msgCheckRect);
			} else {
				iconRect = &(selected ? st::msgSelectDblCheckRect : st::msgDblCheckRect);
			}
		} else {
			iconRect = &st::msgSendingRect;
		}
		p.drawPixmap(iconPos, App::sprite(), *iconRect);
	}
}

void HistoryContact::updateFrom(const MTPMessageMedia &media) {
	if (media.type() == mtpc_messageMediaContact) {
		userId = media.c_messageMediaContact().vuser_id.v;
		contact = App::userLoaded(userId);
		if (contact) {
			if (contact->phone.isEmpty()) {
				contact->setPhone(phone);
			}
			if (contact->contact < 0) {
				contact->contact = 0;
			}
			contact->photo->load();
		}
	}
}

HistoryWebPage::HistoryWebPage(WebPageData *data) : HistoryMedia()
, data(data)
, _openl(0)
, _attachl(0)
, _asArticle(false)
, _title(st::msgMinWidth - st::webPageLeft)
, _description(st::msgMinWidth - st::webPageLeft)
, _siteNameWidth(0)
, _durationWidth(0)
, _docNameWidth(0)
, _docThumbWidth(0)
, _docDownloadDone(0)
, _pixw(0), _pixh(0)
{
}

void HistoryWebPage::initDimensions(const HistoryItem *parent) {
	if (data->pendingTill) {
		_maxw = _minh = _height = 0;
		//_maxw = st::webPageLeft + st::linkFont->m.width(lang((data->pendingTill < 0) ? lng_attach_failed : lng_profile_loading));
		//_minh = st::replyHeight;
		//_height = _minh;
		return;
	}
	if (!_openl && !data->url.isEmpty()) _openl = TextLinkPtr(new TextLink(data->url));
	if (!_attachl && data->photo && data->type != WebPageVideo) _attachl = TextLinkPtr(new PhotoLink(data->photo));
	if (!_attachl && data->doc) _attachl = TextLinkPtr(new DocumentOpenLink(data->doc));

	if (data->photo && data->type != WebPagePhoto && data->type != WebPageVideo) {
		if (data->type == WebPageProfile) {
			_asArticle = true;
		} else if (data->siteName == qstr("Twitter") || data->siteName == qstr("Facebook")) {
			_asArticle = false;
		} else {
			_asArticle = true;
		}
	} else {
		_asArticle = false;
	}

	if (_asArticle) {
		w = st::webPagePhotoSize;
		_maxw = st::webPageLeft + st::webPagePhotoSize;
		_minh = st::webPagePhotoSize;
		_minh += st::webPagePhotoSkip + (st::msgDateFont->height - st::msgDateDelta.y());
	} else if (data->photo) {
		int32 tw = convertScale(data->photo->full->width()), th = convertScale(data->photo->full->height());
		if (!tw || !th) {
			tw = th = 1;
		}
		if (tw > st::maxMediaSize) {
			th = (st::maxMediaSize * th) / tw;
			tw = st::maxMediaSize;
		}
		if (th > st::maxMediaSize) {
			tw = (st::maxMediaSize * tw) / th;
			th = st::maxMediaSize;
		}
		int32 thumbw = tw;
		int32 thumbh = th;

		w = thumbw;

		_maxw = st::webPageLeft + qMax(thumbh, qMax(w, int32(st::minPhotoSize))) + parent->timeWidth(true);
		_minh = qMax(thumbh, int32(st::minPhotoSize));
		_minh += st::webPagePhotoSkip;
	} else if (data->doc) {
		if (!data->doc->thumb->isNull()) {
			data->doc->thumb->load();

			int32 tw = data->doc->thumb->width(), th = data->doc->thumb->height();
			if (data->doc->thumb->isNull() || !tw || !th) {
				_docThumbWidth = 0;
			} else if (tw > th) {
				_docThumbWidth = (tw * st::mediaThumbSize) / th;
			} else {
				_docThumbWidth = st::mediaThumbSize;
			}
		}
		_docName = documentName(data->doc);
		_docSize = data->doc->song() ? formatDurationAndSizeText(data->doc->song()->duration, data->doc->size) : formatSizeText(data->doc->size);
		_docNameWidth = st::mediaFont->m.width(_docName.isEmpty() ? qsl("Document") : _docName);

		if (parent == animated.msg) {
			_maxw = st::webPageLeft + (animated.w / cIntRetinaFactor()) + parent->timeWidth(true);
			_minh = animated.h / cIntRetinaFactor();
			_minh += st::webPagePhotoSkip;
		} else {
			_maxw = qMax(st::webPageLeft + st::mediaThumbSize + st::mediaPadding.right() + _docNameWidth + st::mediaPadding.right(), st::mediaMaxWidth);
			_minh = st::mediaThumbSize;
		}
	} else {
		_maxw = st::webPageLeft;
		_minh = 0;
	}

	if (!data->siteName.isEmpty()) {
		_siteNameWidth = st::webPageTitleFont->m.width(data->siteName);
		if (_asArticle) {
			_maxw = qMax(_maxw, int32(st::webPageLeft + _siteNameWidth + st::webPagePhotoDelta + st::webPagePhotoSize));
		} else {
			_maxw = qMax(_maxw, int32(st::webPageLeft + _siteNameWidth + parent->timeWidth(true)));
			_minh += st::webPageTitleFont->height;
		}
	}
	QString title(data->title.isEmpty() ? data->author : data->title);
	if (!title.isEmpty()) {
		title = textClean(title);
		if (!_asArticle && !data->photo && data->description.isEmpty()) title += textcmdSkipBlock(parent->timeWidth(true), st::msgDateFont->height - st::msgDateDelta.y());
		_title.setText(st::webPageTitleFont, title, _webpageTitleOptions);
		if (_asArticle) {
			_maxw = qMax(_maxw, int32(st::webPageLeft + _title.maxWidth() + st::webPagePhotoDelta + st::webPagePhotoSize));
		} else {
			_maxw = qMax(_maxw, int32(st::webPageLeft + _title.maxWidth()));
			_minh += qMin(_title.minHeight(), 2 * st::webPageTitleFont->height);
		}
	}
	if (!data->description.isEmpty()) {
		QString text = textClean(data->description);
		if (!_asArticle && !data->photo) text += textcmdSkipBlock(parent->timeWidth(true), st::msgDateFont->height - st::msgDateDelta.y());
		const TextParseOptions *opts = &_webpageDescriptionOptions;
		if (data->siteName == qstr("Twitter")) {
			opts = &_twitterDescriptionOptions;
		} else if (data->siteName == qstr("Instagram")) {
			opts = &_instagramDescriptionOptions;
		}
		_description.setText(st::webPageDescriptionFont, text, *opts);
		if (_asArticle) {
			_maxw = qMax(_maxw, int32(st::webPageLeft + _description.maxWidth() + st::webPagePhotoDelta + st::webPagePhotoSize));
		} else {
			_maxw = qMax(_maxw, int32(st::webPageLeft + _description.maxWidth()));
			_minh += qMin(_description.minHeight(), 3 * st::webPageTitleFont->height);
		}
	}
	if (!_asArticle && (data->photo || data->doc) && (_siteNameWidth || !_title.isEmpty() || !_description.isEmpty())) {
		_minh += st::webPagePhotoSkip;
	}
	if (data->type == WebPageVideo && data->duration) {
		_duration = formatDurationText(data->duration);
		_durationWidth = st::msgDateFont->m.width(_duration);
	}
	_height = _minh;
}

void HistoryWebPage::draw(QPainter &p, const HistoryItem *parent, bool selected, int32 width) const {
	if (width < 0) width = w;
	if (width < 1 || data->pendingTill) return;

	int16 animw = 0, animh = 0;
	if (data->doc && animated.msg == parent) {
		animw = animated.w / cIntRetinaFactor();
		animh = animated.h / cIntRetinaFactor();
		if (width - st::webPageLeft < animw) {
			animw = width - st::webPageLeft;
			animh = (animw * animated.h / animated.w);
			if (animh < 1) animh = 1;
		}
	}

	int32 bottomSkip = 0;
	if (data->photo) {
		bottomSkip += st::webPagePhotoSkip;
		if (_asArticle || (st::webPageLeft + qMax(_pixw, int16(st::minPhotoSize)) + parent->timeWidth(true) > width)) {
			bottomSkip += (st::msgDateFont->height - st::msgDateDelta.y());
		}
	} else if (data->doc && animated.msg == parent) {
		bottomSkip += st::webPagePhotoSkip;
		if (st::webPageLeft + qMax(animw, int16(st::minPhotoSize)) + parent->timeWidth(true) > width) {
			bottomSkip += (st::msgDateFont->height - st::msgDateDelta.y());
		}
	}

	style::color bar = (selected ? (parent->out() ? st::msgOutReplyBarSelColor : st::msgInReplyBarSelColor) : (parent->out() ? st::msgOutReplyBarColor : st::msgInReplyBarColor));
	style::color semibold = (selected ? (parent->out() ? st::msgOutServiceSelColor : st::msgInServiceSelColor) : (parent->out() ? st::msgOutServiceColor : st::msgInServiceColor));
	style::color regular = (selected ? (parent->out() ? st::msgOutSelectDateColor : st::msgInSelectDateColor) : (parent->out() ? st::msgOutDateColor : st::msgInDateColor));
	p.fillRect(0, 0, st::webPageBar, _height - bottomSkip, bar->b);

	//if (data->pendingTill) {
	//	p.setFont(st::linkFont->f);
	//	p.setPen(regular->p);
	//	p.drawText(st::webPageLeft, (_minh - st::linkFont->height) / 2 + st::linkFont->ascent, lang(data->pendingTill < 0 ? lng_attach_failed : lng_profile_loading));
	//	return;
	//}

	p.save();
	p.translate(st::webPageLeft, 0);

	width -= st::webPageLeft;

	if (_asArticle) {
		int32 pixwidth = st::webPagePhotoSize, pixheight = st::webPagePhotoSize;
		data->photo->medium->load(false, false);
		bool out = parent->out();
		bool full = data->photo->medium->loaded();
		QPixmap pix;
		if (full) {
			pix = data->photo->medium->pixSingle(_pixw, _pixh, pixwidth, pixheight);
		} else {
			pix = data->photo->thumb->pixBlurredSingle(_pixw, _pixh, pixwidth, pixheight);
		}
		p.drawPixmap(width - pixwidth, 0, pix);
		if (selected) {
			App::roundRect(p, width - pixwidth, 0, pixwidth, pixheight, textstyleCurrent()->selectOverlay, SelectedOverlayCorners);
		}
	}
	int32 articleLines = 5;
	if (_siteNameWidth) {
		int32 availw = width;
		if (_asArticle) {
			availw -= st::webPagePhotoSize + st::webPagePhotoDelta;
		} else if (_title.isEmpty() && _description.isEmpty() && !data->photo) {
			availw -= parent->timeWidth(true);
		}
		p.setFont(st::webPageTitleFont->f);
		p.setPen(semibold->p);
		p.drawText(0, st::webPageTitleFont->ascent, (availw >= _siteNameWidth) ? data->siteName : st::webPageTitleFont->m.elidedText(data->siteName, Qt::ElideRight, availw));
		p.translate(0, st::webPageTitleFont->height);
		--articleLines;
	}
	if (!_title.isEmpty()) {
		p.setPen(st::black->p);
		int32 availw = width, endskip = 0;
		if (_asArticle) {
			availw -= st::webPagePhotoSize + st::webPagePhotoDelta;
		} else if (_description.isEmpty() && !data->photo) {
			endskip = parent->timeWidth(true);
		}
		_title.drawElided(p, 0, 0, availw, 2, style::al_left, 0, -1, endskip);
		int32 h = _title.countHeight(availw);
		if (h > st::webPageTitleFont->height) {
			articleLines -= 2;
			p.translate(0, st::webPageTitleFont->height * 2);
		} else {
			--articleLines;
			p.translate(0, h);
		}
	}
	if (!_description.isEmpty()) {
		p.setPen(st::black->p);
		int32 availw = width, endskip = 0;
		if (_asArticle) {
			availw -= st::webPagePhotoSize + st::webPagePhotoDelta;
			if (articleLines > 3) articleLines = 3;
		} else {
			if (!data->photo) endskip = parent->timeWidth(true);
			articleLines = 3;
		}
		_description.drawElided(p, 0, 0, availw, articleLines, style::al_left, 0, -1, endskip);
		p.translate(0, qMin(_description.countHeight(availw), st::webPageDescriptionFont->height * articleLines));
	}
	if (!_asArticle && data->photo) {
		if (_siteNameWidth || !_title.isEmpty() || !_description.isEmpty()) {
			p.translate(0, st::webPagePhotoSkip);
		}

		int32 pixwidth = qMax(_pixw, int16(st::minPhotoSize)), pixheight = qMax(_pixh, int16(st::minPhotoSize));
		data->photo->full->load(false, false);
		bool out = parent->out();
		bool full = data->photo->full->loaded();
		QPixmap pix;
		if (full) {
			pix = data->photo->full->pixSingle(_pixw, _pixh, pixwidth, pixheight);
		} else {
			pix = data->photo->thumb->pixBlurredSingle(_pixw, _pixh, pixwidth, pixheight);
		}
		p.drawPixmap(0, 0, pix);
		if (!full) {
			uint64 dt = itemAnimations().animate(parent, getms());
			int32 cnt = int32(st::photoLoaderCnt), period = int32(st::photoLoaderPeriod), t = dt % period, delta = int32(st::photoLoaderDelta);

			int32 x = (pixwidth - st::photoLoader.width()) / 2, y = (pixheight - st::photoLoader.height()) / 2;
			p.fillRect(x, y, st::photoLoader.width(), st::photoLoader.height(), st::photoLoaderBg->b);
			x += (st::photoLoader.width() - cnt * st::photoLoaderPoint.width() - (cnt - 1) * st::photoLoaderSkip) / 2;
			y += (st::photoLoader.height() - st::photoLoaderPoint.height()) / 2;
			QColor c(st::white->c);
			QBrush b(c);
			for (int32 i = 0; i < cnt; ++i) {
				t -= delta;
				while (t < 0) t += period;

				float64 alpha = (t >= st::photoLoaderDuration1 + st::photoLoaderDuration2) ? 0 : ((t > st::photoLoaderDuration1 ? ((st::photoLoaderDuration1 + st::photoLoaderDuration2 - t) / st::photoLoaderDuration2) : (t / st::photoLoaderDuration1)));
				c.setAlphaF(st::photoLoaderAlphaMin + alpha * (1 - st::photoLoaderAlphaMin));
				b.setColor(c);
				p.fillRect(x + i * (st::photoLoaderPoint.width() + st::photoLoaderSkip), y, st::photoLoaderPoint.width(), st::photoLoaderPoint.height(), b);
			}
		}

		if (selected) {
			App::roundRect(p, 0, 0, pixwidth, pixheight, textstyleCurrent()->selectOverlay, SelectedOverlayCorners);
		}

		if (data->type == WebPageVideo) {
			if (data->siteName == qstr("YouTube")) {
				p.drawPixmap(QPoint((pixwidth - st::youtubeIcon.pxWidth()) / 2, (pixheight - st::youtubeIcon.pxHeight()) / 2), App::sprite(), st::youtubeIcon);
			} else {
				p.drawPixmap(QPoint((pixwidth - st::videoIcon.pxWidth()) / 2, (pixheight - st::videoIcon.pxHeight()) / 2), App::sprite(), st::videoIcon);
			}
			if (_durationWidth) {
				int32 dateX = pixwidth - _durationWidth - st::msgDateImgDelta - 2 * st::msgDateImgPadding.x();
				int32 dateY = pixheight - st::msgDateFont->height - 2 * st::msgDateImgPadding.y() - st::msgDateImgDelta;
				int32 dateW = pixwidth - dateX - st::msgDateImgDelta;
				int32 dateH = pixheight - dateY - st::msgDateImgDelta;

				App::roundRect(p, dateX, dateY, dateW, dateH, selected ? st::msgDateImgSelectBg : st::msgDateImgBg, selected ? DateSelectedCorners : DateCorners);

				p.setFont(st::msgDateFont->f);
				p.setPen(st::msgDateImgColor->p);
				p.drawText(dateX + st::msgDateImgPadding.x(), dateY + st::msgDateImgPadding.y() + st::msgDateFont->ascent, _duration);
			}
		}

		p.translate(0, pixheight);
	} else if (!_asArticle && data->doc) {
		if (_siteNameWidth || !_title.isEmpty() || !_description.isEmpty()) {
			p.translate(0, st::webPagePhotoSkip);
		}

		if (parent == animated.msg) {
			p.drawPixmap(0, 0, animated.current(animw * cIntRetinaFactor(), animh * cIntRetinaFactor(), true));
			if (selected) {
				App::roundRect(p, 0, 0, animw, animh, textstyleCurrent()->selectOverlay, SelectedOverlayCorners);
			}
		} else {
			QString statusText;
			if (data->doc->song()) {
				SongMsgId playing;
				AudioPlayerState playingState = AudioPlayerStopped;
				int64 playingPosition = 0, playingDuration = 0;
				int32 playingFrequency = 0;
				if (audioPlayer()) {
					audioPlayer()->currentState(&playing, &playingState, &playingPosition, &playingDuration, &playingFrequency);
				}

				bool already = !data->doc->already().isEmpty(), hasdata = !data->doc->data.isEmpty();
				QRect img;
				if (data->doc->status == FileFailed) {
					statusText = lang(lng_attach_failed);
					img = parent->out() ? st::mediaMusicOutImg : st::mediaMusicInImg;
				} else if (already || hasdata) {
					bool showPause = false;
					if (playing.msgId == parent->id && !(playingState & AudioPlayerStoppedMask) && playingState != AudioPlayerFinishing) {
						statusText = formatDurationText(playingPosition / (playingFrequency ? playingFrequency : AudioVoiceMsgFrequency)) + qsl(" / ") + formatDurationText(playingDuration / (playingFrequency ? playingFrequency : AudioVoiceMsgFrequency));
						showPause = (playingState == AudioPlayerPlaying || playingState == AudioPlayerResuming || playingState == AudioPlayerStarting);
					} else {
						statusText = formatDurationText(data->doc->song()->duration);
					}
					if (!showPause && playing.msgId == parent->id && App::main() && App::main()->player()->seekingSong(playing)) showPause = true;
					img = parent->out() ? (showPause ? st::mediaPauseOutImg : st::mediaPlayOutImg) : (showPause ? st::mediaPauseInImg : st::mediaPlayInImg);
				} else {
					if (data->doc->loader) {
						int32 offset = data->doc->loader->currentOffset();
						if (_docDownloadTextCache.isEmpty() || _docDownloadDone != offset) {
							_docDownloadDone = offset;
							_docDownloadTextCache = formatDownloadText(_docDownloadDone, data->doc->size);
						}
						statusText = _docDownloadTextCache;
					} else {
						statusText = _docSize;
					}
					img = parent->out() ? st::mediaMusicOutImg : st::mediaMusicInImg;
				}

				p.drawPixmap(QPoint(0, 0), App::sprite(), img);
			} else {
				if (data->doc->status == FileFailed) {
					statusText = lang(lng_attach_failed);
				} else if (data->doc->loader) {
					int32 offset = data->doc->loader->currentOffset();
					if (_docDownloadTextCache.isEmpty() || _docDownloadDone != offset) {
						_docDownloadDone = offset;
						_docDownloadTextCache = formatDownloadText(_docDownloadDone, data->doc->size);
					}
					statusText = _docDownloadTextCache;
				} else {
					statusText = _docSize;
				}

				if (_docThumbWidth) {
					data->doc->thumb->checkload();
					p.drawPixmap(QPoint(0, 0), data->doc->thumb->pixSingle(_docThumbWidth, 0, st::mediaThumbSize, st::mediaThumbSize));
				} else {
					p.drawPixmap(QPoint(0, 0), App::sprite(), (parent->out() ? st::mediaDocOutImg : st::mediaDocInImg));
				}
			}
			if (selected) {
				App::roundRect(p, 0, 0, st::mediaThumbSize, st::mediaThumbSize, textstyleCurrent()->selectOverlay, SelectedOverlayCorners);
			}

			int32 tleft = st::mediaPadding.left() + st::mediaThumbSize + st::mediaPadding.right();
			int32 twidth = width - tleft - st::mediaPadding.right();
			int32 fullTimeWidth = parent->timeWidth(true) + st::msgPadding.right();
			int32 secondwidth = width - tleft - fullTimeWidth;

			p.setFont(st::mediaFont->f);
			p.setPen(st::black->c);
			if (twidth < _docNameWidth) {
				p.drawText(tleft, st::mediaNameTop + st::mediaFont->ascent, st::mediaFont->m.elidedText(_docName, Qt::ElideRight, twidth));
			} else {
				p.drawText(tleft, st::mediaNameTop + st::mediaFont->ascent, _docName);
			}

			style::color status(selected ? (parent->out() ? st::mediaOutSelectColor : st::mediaInSelectColor) : (parent->out() ? st::mediaOutColor : st::mediaInColor));
			p.setPen(status->p);

			p.drawText(tleft, st::mediaThumbSize - st::mediaDetailsShift - st::mediaFont->descent, statusText);
		}
	}

	p.restore();
}

int32 HistoryWebPage::resize(int32 width, bool dontRecountText, const HistoryItem *parent) {
	if (data->pendingTill) {
		w = width;
		_height = _minh;
		return _height;
	}

	w = width;
	width -= st::webPageLeft;
	if (_asArticle) {
		int32 tw = convertScale(data->photo->medium->width()), th = convertScale(data->photo->medium->height());
		if (tw > st::webPagePhotoSize) {
			if (th > tw) {
				th = th * st::webPagePhotoSize / tw;
				tw = st::webPagePhotoSize;
			} else if (th > st::webPagePhotoSize) {
				tw = tw * st::webPagePhotoSize / th;
				th = st::webPagePhotoSize;
			}
		}
		_pixw = tw;
		_pixh = th;
		if (_pixw < 1) _pixw = 1;
		if (_pixh < 1) _pixh = 1;
		_height = st::webPagePhotoSize;
		_height += st::webPagePhotoSkip + (st::msgDateFont->height - st::msgDateDelta.y());
	} else if (data->photo) {
		_pixw = qMin(width, int32(_maxw - st::webPageLeft - parent->timeWidth(true)));

		int32 tw = convertScale(data->photo->full->width()), th = convertScale(data->photo->full->height());
		if (tw > st::maxMediaSize) {
			th = (st::maxMediaSize * th) / tw;
			tw = st::maxMediaSize;
		}
		if (th > st::maxMediaSize) {
			tw = (st::maxMediaSize * tw) / th;
			th = st::maxMediaSize;
		}
		_pixh = th;
		if (tw > _pixw) {
			_pixh = (_pixw * _pixh / tw);
		} else {
			_pixw = tw;
		}
		if (_pixh > width) {
			_pixw = (_pixw * width) / _pixh;
			_pixh = width;
		}
		if (_pixw < 1) _pixw = 1;
		if (_pixh < 1) _pixh = 1;
		_height = qMax(_pixh, int16(st::minPhotoSize));
		_height += st::webPagePhotoSkip;
		if (qMax(_pixw, int16(st::minPhotoSize)) + parent->timeWidth(true) > width) {
			_height += (st::msgDateFont->height - st::msgDateDelta.y());
		}
	} else if (data->doc) {
		if (parent == animated.msg) {
			int32 w = qMin(width, int32(animated.w / cIntRetinaFactor()));
			if (w > st::maxMediaSize) {
				w = st::maxMediaSize;
			}
			_height = animated.h / cIntRetinaFactor();
			if (animated.w / cIntRetinaFactor() > w) {
				_height = (w * _height / (animated.w / cIntRetinaFactor()));
				if (_height <= 0) _height = 1;
			}
			_height += st::webPagePhotoSkip;
			if (w + parent->timeWidth(true) > width) {
				_height += (st::msgDateFont->height - st::msgDateDelta.y());
			}
		} else {
			_height = st::mediaThumbSize;
		}
	} else {
		_height = 0;
	}

	if (!_asArticle) {
		if (!data->siteName.isEmpty()) {
			_height += st::webPageTitleFont->height;
		}
		if (!_title.isEmpty()) {
			_height += qMin(_title.countHeight(width), st::webPageTitleFont->height * 2);
		}
		if (!_description.isEmpty()) {
			_height += qMin(_description.countHeight(width), st::webPageDescriptionFont->height * 3);
		}
		if ((data->photo || data->doc) && (_siteNameWidth || !_title.isEmpty() || !_description.isEmpty())) {
			_height += st::webPagePhotoSkip;
		}
	}

	return _height;
}

void HistoryWebPage::regItem(HistoryItem *item) {
	App::regWebPageItem(data, item);
	if (data->doc) App::regDocumentItem(data->doc, item);
}

void HistoryWebPage::unregItem(HistoryItem *item) {
	App::unregWebPageItem(data, item);
	if (data->doc) App::unregDocumentItem(data->doc, item);
}

const QString HistoryWebPage::inDialogsText() const {
	return QString();
}

const QString HistoryWebPage::inHistoryText() const {
	return QString();
}

bool HistoryWebPage::hasPoint(int32 x, int32 y, const HistoryItem *parent, int32 width) const {
	if (width < 0) width = w;
	if (width >= _maxw) width = _maxw;

	return (x >= 0 && y >= 0 && x < width && y < _height);
}

void HistoryWebPage::getState(TextLinkPtr &lnk, HistoryCursorState &state, int32 x, int32 y, const HistoryItem *parent, int32 width) const {
	if (width < 0) width = w;
	if (width < 1) return;

	width -= st::webPageLeft;
	x -= st::webPageLeft;

	if (_asArticle) {
		int32 pixwidth = st::webPagePhotoSize, pixheight = st::webPagePhotoSize;
		if (x >= width - pixwidth && x < width && y >= 0 && y < pixheight) {
			lnk = _openl;
			return;
		}
	}
	int32 articleLines = 5;
	if (_siteNameWidth) {
		y -= st::webPageTitleFont->height;
		--articleLines;
	}
	if (!_title.isEmpty()) {
		int32 availw = width;
		if (_asArticle) {
			availw -= st::webPagePhotoSize + st::webPagePhotoDelta;
		}
		int32 h = _title.countHeight(availw);
		if (h > st::webPageTitleFont->height) {
			articleLines -= 2;
			y -= st::webPageTitleFont->height * 2;
		} else {
			--articleLines;
			y -= h;
		}
	}
	if (!_description.isEmpty()) {
		int32 availw = width;
		if (_asArticle) {
			availw -= st::webPagePhotoSize + st::webPagePhotoDelta;
			if (articleLines > 3) articleLines = 3;
		} else if (!data->photo) {
			articleLines = 3;
		}
		int32 desch = qMin(_description.countHeight(width), st::webPageDescriptionFont->height * articleLines);
		if (y >= 0 && y < desch) {
			bool inText = false;
			_description.getState(lnk, inText, x, y, availw);
			state = inText ? HistoryInTextCursorState : HistoryDefaultCursorState;
			return;
		}
		y -= desch;
	}
	if (_siteNameWidth || !_title.isEmpty() || !_description.isEmpty()) {
		y -= st::webPagePhotoSkip;
	}
	if (!_asArticle) {
		if (data->doc && parent == animated.msg) {
			int32 h = (width == w) ? _height : (width * animated.h / animated.w);
			if (h < 1) h = 1;
			if (x >= 0 && y >= 0 && x < width && y < h) {
				lnk = _attachl;
				return;
			}
		} else {
			int32 attachwidth = data->doc ? (width - st::mediaPadding.right()) : qMax(_pixw, int16(st::minPhotoSize));
			int32 attachheight = data->doc ? st::mediaThumbSize : qMax(_pixh, int16(st::minPhotoSize));
			if (x >= 0 && y >= 0 && x < attachwidth && y < attachheight) {
				lnk = _attachl ? _attachl : _openl;
				return;
			}
		}
	}
}

HistoryMedia *HistoryWebPage::clone() const {
	return new HistoryWebPage(*this);
}

ImagePtr HistoryWebPage::replyPreview() {
	return data->photo ? data->photo->makeReplyPreview() : (data->doc ? data->doc->makeReplyPreview() : ImagePtr());
}

namespace {
	QRegularExpression reYouTube1(qsl("^(https?://)?(www\\.|m\\.)?youtube\\.com/watch\\?([^#]+&)?v=([a-z0-9_-]+)(&[^\\s]*)?$"), QRegularExpression::CaseInsensitiveOption);
    QRegularExpression reYouTube2(qsl("^(https?://)?(www\\.)?youtu\\.be/([a-z0-9_-]+)([/\\?#][^\\s]*)?$"), QRegularExpression::CaseInsensitiveOption);
	QRegularExpression reInstagram(qsl("^(https?://)?(www\\.)?instagram\\.com/p/([a-z0-9_-]+)([/\\?][^\\s]*)?$"), QRegularExpression::CaseInsensitiveOption);
	QRegularExpression reVimeo(qsl("^(https?://)?(www\\.)?vimeo\\.com/([0-9]+)([/\\?][^\\s]*)?$"), QRegularExpression::CaseInsensitiveOption);

	ImageLinkManager manager;
}

void ImageLinkManager::init() {
	if (manager) delete manager;
	manager = new QNetworkAccessManager();
	App::setProxySettings(*manager);

	connect(manager, SIGNAL(authenticationRequired(QNetworkReply*, QAuthenticator*)), this, SLOT(onFailed(QNetworkReply*)));
	connect(manager, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError>&)), this, SLOT(onFailed(QNetworkReply*)));
	connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(onFinished(QNetworkReply*)));

	if (black) delete black;
	QImage b(cIntRetinaFactor(), cIntRetinaFactor(), QImage::Format_ARGB32_Premultiplied);
	{
		QPainter p(&b);
		p.fillRect(QRect(0, 0, cIntRetinaFactor(), cIntRetinaFactor()), st::white->b);
	}
	QPixmap p = QPixmap::fromImage(b, Qt::ColorOnly);
	p.setDevicePixelRatio(cRetinaFactor());
	black = new ImagePtr(p, "PNG");
}

void ImageLinkManager::reinit() {
	if (manager) App::setProxySettings(*manager);
}

void ImageLinkManager::deinit() {
	if (manager) {
		delete manager;
		manager = 0;
	}
	if (black) {
		delete black;
		black = 0;
	}
	dataLoadings.clear();
	imageLoadings.clear();
}

void initImageLinkManager() {
	manager.init();
}

void reinitImageLinkManager() {
	manager.reinit();
}

void deinitImageLinkManager() {
	manager.deinit();
}

void ImageLinkManager::getData(ImageLinkData *data) {
	if (!manager) {
		DEBUG_LOG(("App Error: getting image link data without manager init!"));
		return failed(data);
	}
	QString url;
	switch (data->type) {
	case YouTubeLink: {
		url = qsl("https://gdata.youtube.com/feeds/api/videos/") + data->id.mid(8) + qsl("?v=2&alt=json");
		QNetworkReply *reply = manager->get(QNetworkRequest(QUrl(url)));
		dataLoadings[reply] = data;
	} break;
	case VimeoLink: {
		url = qsl("https://vimeo.com/api/v2/video/") + data->id.mid(6) + qsl(".json");
		QNetworkReply *reply = manager->get(QNetworkRequest(QUrl(url)));
		dataLoadings[reply] = data;
	} break;
	case InstagramLink: {
		//url = qsl("https://api.instagram.com/oembed?url=http://instagr.am/p/") + data->id.mid(10) + '/';
		url = qsl("https://instagram.com/p/") + data->id.mid(10) + qsl("/media/?size=l");
		QNetworkReply *reply = manager->get(QNetworkRequest(QUrl(url)));
		imageLoadings[reply] = data;
	} break;
	case GoogleMapsLink: {
		int32 w = st::locationSize.width(), h = st::locationSize.height();
		int32 zoom = 13, scale = 1;
		if (cScale() == dbisTwo || cRetina()) {
			scale = 2;
		} else {
			w = convertScale(w);
			h = convertScale(h);
		}
		url = qsl("https://maps.googleapis.com/maps/api/staticmap?center=") + data->id.mid(9) + qsl("&zoom=%1&size=%2x%3&maptype=roadmap&scale=%4&markers=color:red|size:big|").arg(zoom).arg(w).arg(h).arg(scale) + data->id.mid(9) + qsl("&sensor=false");
		QNetworkReply *reply = manager->get(QNetworkRequest(QUrl(url)));
		imageLoadings[reply] = data;
	} break;
	default: {
		failed(data);
	} break;
	}
}

void ImageLinkManager::onFinished(QNetworkReply *reply) {
	if (!manager) return;
	if (reply->error() != QNetworkReply::NoError) return onFailed(reply);

	QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
	if (statusCode.isValid()) {
		int status = statusCode.toInt();
		if (status == 301 || status == 302) {
			QString loc = reply->header(QNetworkRequest::LocationHeader).toString();
			if (!loc.isEmpty()) {
				QMap<QNetworkReply*, ImageLinkData*>::iterator i = dataLoadings.find(reply);
				if (i != dataLoadings.cend()) {
					ImageLinkData *d = i.value();
					if (serverRedirects.constFind(d) == serverRedirects.cend()) {
						serverRedirects.insert(d, 1);
					} else if (++serverRedirects[d] > MaxHttpRedirects) {
						DEBUG_LOG(("Network Error: Too many HTTP redirects in onFinished() for image link: %1").arg(loc));
						return onFailed(reply);
					}
					dataLoadings.erase(i);
					dataLoadings.insert(manager->get(QNetworkRequest(loc)), d);
					return;
				} else if ((i = imageLoadings.find(reply)) != imageLoadings.cend()) {
					ImageLinkData *d = i.value();
					if (serverRedirects.constFind(d) == serverRedirects.cend()) {
						serverRedirects.insert(d, 1);
					} else if (++serverRedirects[d] > MaxHttpRedirects) {
						DEBUG_LOG(("Network Error: Too many HTTP redirects in onFinished() for image link: %1").arg(loc));
						return onFailed(reply);
					}
					imageLoadings.erase(i);
					imageLoadings.insert(manager->get(QNetworkRequest(loc)), d);
					return;
				}
			}
		}
		if (status != 200) {
			DEBUG_LOG(("Network Error: Bad HTTP status received in onFinished() for image link: %1").arg(status));
			return onFailed(reply);
		}
	}

	ImageLinkData *d = 0;
	QMap<QNetworkReply*, ImageLinkData*>::iterator i = dataLoadings.find(reply);
	if (i != dataLoadings.cend()) {
		d = i.value();
		dataLoadings.erase(i);

		QJsonParseError e;
		QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &e);
		if (e.error != QJsonParseError::NoError) {
			DEBUG_LOG(("JSON Error: Bad json received in onFinished() for image link"));
			return onFailed(reply);
		}
		switch (d->type) {
		case YouTubeLink: {
			QJsonObject obj = doc.object();
			QString thumb;
			int32 seconds = 0;
			QJsonObject::const_iterator entryIt = obj.constFind(qsl("entry"));
			if (entryIt != obj.constEnd() && entryIt.value().isObject()) {
				QJsonObject entry = entryIt.value().toObject();
				QJsonObject::const_iterator mediaIt = entry.constFind(qsl("media$group"));
				if (mediaIt != entry.constEnd() && mediaIt.value().isObject()) {
					QJsonObject media = mediaIt.value().toObject();

					// title from media
					QJsonObject::const_iterator titleIt = media.constFind(qsl("media$title"));
					if (titleIt != media.constEnd() && titleIt.value().isObject()) {
						QJsonObject title = titleIt.value().toObject();
						QJsonObject::const_iterator tIt = title.constFind(qsl("$t"));
						if (tIt != title.constEnd() && tIt.value().isString()) {
							d->title = tIt.value().toString();
						}
					}

					// thumb
					QJsonObject::const_iterator thumbnailsIt = media.constFind(qsl("media$thumbnail"));
					int32 bestLevel = 0;
					if (thumbnailsIt != media.constEnd() && thumbnailsIt.value().isArray()) {
						QJsonArray thumbnails = thumbnailsIt.value().toArray();
						for (int32 i = 0, l = thumbnails.size(); i < l; ++i) {
							QJsonValue thumbnailVal = thumbnails.at(i);
							if (!thumbnailVal.isObject()) continue;
							
							QJsonObject thumbnail = thumbnailVal.toObject();
							QJsonObject::const_iterator urlIt = thumbnail.constFind(qsl("url"));
							if (urlIt == thumbnail.constEnd() || !urlIt.value().isString()) continue;

							int32 level = 0;
							if (thumbnail.constFind(qsl("time")) == thumbnail.constEnd()) {
								level += 10;
							}
							QJsonObject::const_iterator wIt = thumbnail.constFind(qsl("width"));
							if (wIt != thumbnail.constEnd()) {
								int32 w = 0;
								if (wIt.value().isDouble()) {
									w = qMax(qRound(wIt.value().toDouble()), 0);
								} else if (wIt.value().isString()) {
									w = qMax(qRound(wIt.value().toString().toDouble()), 0);
								}
								switch (w) {
								case 640: level += 4; break;
								case 480: level += 3; break;
								case 320: level += 2; break;
								case 120: level += 1; break;
								}
							}
							if (level > bestLevel) {
								thumb = urlIt.value().toString();
								bestLevel = level;
							}
						}
					}

					// duration
					QJsonObject::const_iterator durationIt = media.constFind(qsl("yt$duration"));
					if (durationIt != media.constEnd() && durationIt.value().isObject()) {
						QJsonObject duration = durationIt.value().toObject();
						QJsonObject::const_iterator secondsIt = duration.constFind(qsl("seconds"));
						if (secondsIt != duration.constEnd()) {
							if (secondsIt.value().isDouble()) {
								seconds = qRound(secondsIt.value().toDouble());
							} else if (secondsIt.value().isString()) {
								seconds = qRound(secondsIt.value().toString().toDouble());
							}
						}
					}
				}

				// title field
				if (d->title.isEmpty()) {
					QJsonObject::const_iterator titleIt = entry.constFind(qsl("title"));
					if (titleIt != entry.constEnd() && titleIt.value().isObject()) {
						QJsonObject title = titleIt.value().toObject();
						QJsonObject::const_iterator tIt = title.constFind(qsl("$t"));
						if (tIt != title.constEnd() && tIt.value().isString()) {
							d->title = tIt.value().toString();
						}
					}
				}
			}

			if (seconds > 0) {
				d->duration = formatDurationText(seconds);
			}
			if (thumb.isEmpty()) {
				failed(d);
			} else {
				imageLoadings.insert(manager->get(QNetworkRequest(thumb)), d);
			}
		} break;
		
		case VimeoLink: {
			QString thumb;
			int32 seconds = 0;
			QJsonArray arr = doc.array();
			if (!arr.isEmpty()) {
				QJsonObject obj = arr.at(0).toObject();
				QJsonObject::const_iterator titleIt = obj.constFind(qsl("title"));
				if (titleIt != obj.constEnd() && titleIt.value().isString()) {
					d->title = titleIt.value().toString();
				}
				QJsonObject::const_iterator thumbnailsIt = obj.constFind(qsl("thumbnail_large"));
				if (thumbnailsIt != obj.constEnd() && thumbnailsIt.value().isString()) {
					thumb = thumbnailsIt.value().toString();
				}
				QJsonObject::const_iterator secondsIt = obj.constFind(qsl("duration"));
				if (secondsIt != obj.constEnd()) {
					if (secondsIt.value().isDouble()) {
						seconds = qRound(secondsIt.value().toDouble());
					} else if (secondsIt.value().isString()) {
						seconds = qRound(secondsIt.value().toString().toDouble());
					}
				}
			}
			if (seconds > 0) {
				d->duration = formatDurationText(seconds);
			}
			if (thumb.isEmpty()) {
				failed(d);
			} else {
				imageLoadings.insert(manager->get(QNetworkRequest(thumb)), d);
			}
		} break;

		case InstagramLink: failed(d); break;
		case GoogleMapsLink: failed(d); break;
		}

		if (App::main()) App::main()->update();
	} else {
		i = imageLoadings.find(reply);
		if (i != imageLoadings.cend()) {
			d = i.value();
			imageLoadings.erase(i);

			QPixmap thumb;
			QByteArray format;
			QByteArray data(reply->readAll());
			{
				QBuffer buffer(&data);
				QImageReader reader(&buffer);
				thumb = QPixmap::fromImageReader(&reader, Qt::ColorOnly);
				format = reader.format();
				thumb.setDevicePixelRatio(cRetinaFactor());
				if (format.isEmpty()) format = QByteArray("JPG");
			}
			d->loading = false;
			d->thumb = thumb.isNull() ? (*black) : ImagePtr(thumb, format);
			serverRedirects.remove(d);
			if (App::main()) App::main()->update();
		}
	}
}

void ImageLinkManager::onFailed(QNetworkReply *reply) {
	if (!manager) return;

	ImageLinkData *d = 0;
	QMap<QNetworkReply*, ImageLinkData*>::iterator i = dataLoadings.find(reply);
	if (i != dataLoadings.cend()) {
		d = i.value();
		dataLoadings.erase(i);
	} else {
		i = imageLoadings.find(reply);
		if (i != imageLoadings.cend()) {
			d = i.value();
			imageLoadings.erase(i);
		}
	}
	DEBUG_LOG(("Network Error: failed to get data for image link %1, error %2").arg(d ? d->id : 0).arg(reply->errorString()));
	if (d) {
		failed(d);
	}
}

void ImageLinkManager::failed(ImageLinkData *data) {
	data->loading = false;
	data->thumb = *black;
	serverRedirects.remove(data);
}

void ImageLinkData::load() {
	if (!thumb->isNull()) return thumb->load(false, false);
	if (loading) return;

	loading = true;
	manager.getData(this);
}

HistoryImageLink::HistoryImageLink(const QString &url, const QString &title, const QString &description) : HistoryMedia(),
_title(st::msgMinWidth),
_description(st::msgMinWidth) {
	if (!title.isEmpty()) {
		_title.setText(st::webPageTitleFont, textClean(title), _webpageTitleOptions);
	}
	if (!description.isEmpty()) {
		_description.setText(st::webPageDescriptionFont, textClean(description), _webpageDescriptionOptions);
	}

	if (url.startsWith(qsl("location:"))) {
		QString lnk = qsl("https://maps.google.com/maps?q=") + url.mid(9) + qsl("&ll=") + url.mid(9) + qsl("&z=17");
		link.reset(new TextLink(lnk));

		data = App::imageLinkSet(url, GoogleMapsLink, lnk);
	} else {
		link.reset(new TextLink(url));

		int matchIndex = 4;
		QRegularExpressionMatch m = reYouTube1.match(url);
		if (!m.hasMatch()) {
			m = reYouTube2.match(url);
			matchIndex = 3;
		}
		if (m.hasMatch()) {
			data = App::imageLinkSet(qsl("youtube:") + m.captured(matchIndex), YouTubeLink, url);
		} else {
			m = reVimeo.match(url);
			if (m.hasMatch()) {
				data = App::imageLinkSet(qsl("vimeo:") + m.captured(3), VimeoLink, url);
			} else {
				m = reInstagram.match(url);
				if (m.hasMatch()) {
					data = App::imageLinkSet(qsl("instagram:") + m.captured(3), InstagramLink, url);
					data->title = qsl("instagram.com/p/") + m.captured(3);
				} else {
					data = 0;
				}
			}
		}
	}
}

int32 HistoryImageLink::fullWidth() const {
	if (data) {
		switch (data->type) {
		case YouTubeLink: return 640;
		case VimeoLink: return 640;
		case InstagramLink: return 640;
		case GoogleMapsLink: return st::locationSize.width();
		}
	}
	return st::minPhotoSize;
}

int32 HistoryImageLink::fullHeight() const {
	if (data) {
		switch (data->type) {
		case YouTubeLink: return 480;
		case VimeoLink: return 480;
		case InstagramLink: return 640;
		case GoogleMapsLink: return st::locationSize.height();
		}
	}
	return st::minPhotoSize;
}

void HistoryImageLink::initDimensions(const HistoryItem *parent) {
	int32 tw = convertScale(fullWidth()), th = convertScale(fullHeight());
	int32 thumbw = qMax(tw, int32(st::minPhotoSize)), maxthumbh = thumbw;
	int32 thumbh = qRound(th * float64(thumbw) / tw);
	if (thumbh > maxthumbh) {
		thumbw = qRound(thumbw * float64(maxthumbh) / thumbh);
		thumbh = maxthumbh;
		if (thumbw < st::minPhotoSize) {
			thumbw = st::minPhotoSize;
		}
	}
	if (thumbh < st::minPhotoSize) {
		thumbh = st::minPhotoSize;
	}
	if (!w) {
		w = thumbw;
	}
	_maxw = w;
	_minh = thumbh;
	const HistoryReply *reply = toHistoryReply(parent);
	const HistoryForwarded *fwd = toHistoryForwarded(parent);
	if (reply || !_title.isEmpty() || !_description.isEmpty()) {
		_maxw += st::mediaPadding.left() + st::mediaPadding.right();
		if (reply) {
			_minh += st::msgReplyPadding.top() + st::msgReplyBarSize.height() + st::msgReplyPadding.bottom();
		} else {
			if (parent->out() || !parent->history()->peer->chat || !fwd) {
				_minh += st::msgPadding.top();
			}
			if (fwd) {
				_minh += st::msgServiceNameFont->height + st::msgPadding.top();
			}
		}
		if (!parent->out() && parent->history()->peer->chat) {
			_minh += st::msgPadding.top() + st::msgNameFont->height;
		}
		if (!_title.isEmpty()) {
			_maxw = qMax(_maxw, int32(st::webPageLeft + _title.maxWidth()));
			_minh += qMin(_title.minHeight(), 2 * st::webPageTitleFont->height);
		}
		if (!_description.isEmpty()) {
			_maxw = qMax(_maxw, int32(st::webPageLeft + _description.maxWidth()));
			_minh += qMin(_description.minHeight(), 3 * st::webPageTitleFont->height);
		}
		if (!_title.isEmpty() || !_description.isEmpty()) {
			_minh += st::webPagePhotoSkip;
		}
		_minh += st::mediaPadding.bottom();
	}
	_height = _minh;
}

void HistoryImageLink::draw(QPainter &p, const HistoryItem *parent, bool selected, int32 width) const {
	if (width < 0) width = w;
	int skipx = 0, skipy = 0, height = _height;
	const HistoryReply *reply = toHistoryReply(parent);
	const HistoryForwarded *fwd = toHistoryForwarded(parent);
	bool out = parent->out();

	if (reply || !_title.isEmpty() || !_description.isEmpty()) {
		skipx = st::mediaPadding.left();

		style::color bg(selected ? (out ? st::msgOutSelectBg : st::msgInSelectBg) : (out ? st::msgOutBg : st::msgInBg));
		style::color sh(selected ? (out ? st::msgOutSelectShadow : st::msgInSelectShadow) : (out ? st::msgOutShadow : st::msgInShadow));
		RoundCorners cors(selected ? (out ? MessageOutSelectedCorners : MessageInSelectedCorners) : (out ? MessageOutCorners : MessageInCorners));
		App::roundRect(p, 0, 0, width, _height, bg, cors, &sh);

		int replyFrom = 0, fwdFrom = 0;
		if (!out && parent->history()->peer->chat) {
			replyFrom = st::msgPadding.top() + st::msgNameFont->height;
			fwdFrom = st::msgPadding.top() + st::msgNameFont->height;
			skipy += replyFrom;
			p.setFont(st::msgNameFont->f);
			p.setPen(parent->from()->color->p);
			parent->from()->nameText.drawElided(p, st::mediaPadding.left(), st::msgPadding.top(), width - st::mediaPadding.left() - st::mediaPadding.right());
			if (!fwd && !reply) skipy += st::msgPadding.top();
		} else if (!reply) {
			fwdFrom = st::msgPadding.top();
			skipy += fwdFrom;
		}
		if (reply) {
			skipy += st::msgReplyPadding.top() + st::msgReplyBarSize.height() + st::msgReplyPadding.bottom();
			reply->drawReplyTo(p, st::msgReplyPadding.left(), replyFrom, width - st::msgReplyPadding.left() - st::msgReplyPadding.right(), selected);
		} else if (fwd) {
			skipy += st::msgServiceNameFont->height + st::msgPadding.top();
			fwd->drawForwardedFrom(p, st::mediaPadding.left(), fwdFrom, width - st::mediaPadding.left() - st::mediaPadding.right(), selected);
		}
		width -= st::mediaPadding.left() + st::mediaPadding.right();

		if (!_title.isEmpty()) {
			p.setPen(st::black->p);
			_title.drawElided(p, st::mediaPadding.left(), skipy, width, 2);
			skipy += qMin(_title.countHeight(width), 2 * st::webPageTitleFont->height);
		}
		if (!_description.isEmpty()) {
			p.setPen(st::black->p);
			_description.drawElided(p, st::mediaPadding.left(), skipy, width, 3);
			skipy += qMin(_description.countHeight(width), 3 * st::webPageDescriptionFont->height);
		}
		if (!_title.isEmpty() || !_description.isEmpty()) {
			skipy += st::webPagePhotoSkip;
		}
		height -= skipy + st::mediaPadding.bottom();
	} else {
		App::roundShadow(p, 0, 0, width, _height, selected ? st::msgInSelectShadow : st::msgInShadow, selected ? InSelectedShadowCorners : InShadowCorners);
	}

	data->load();
	QPixmap toDraw;
	if (data && !data->thumb->isNull()) {
		int32 w = data->thumb->width(), h = data->thumb->height();
		QPixmap pix;
		if (width * h == height * w || (w == convertScale(fullWidth()) && h == convertScale(fullHeight()))) {
			pix = data->thumb->pixSingle(width, height, width, height);
		} else if (width * h > height * w) {
			int32 nw = height * w / h;
			pix = data->thumb->pixSingle(nw, height, width, height);
		} else {
			int32 nh = width * h / w;
			pix = data->thumb->pixSingle(width, nh, width, height);
		}
		p.drawPixmap(QPoint(skipx, skipy), pix);
	} else {
		App::roundRect(p, skipx, skipy, width, height, st::black, BlackCorners);
	}
	if (data) {
		switch (data->type) {
		case YouTubeLink: p.drawPixmap(QPoint(skipx + (width - st::youtubeIcon.pxWidth()) / 2, skipy + (height - st::youtubeIcon.pxHeight()) / 2), App::sprite(), st::youtubeIcon); break;
		case VimeoLink: p.drawPixmap(QPoint(skipx + (width - st::vimeoIcon.pxWidth()) / 2, skipy + (height - st::vimeoIcon.pxHeight()) / 2), App::sprite(), st::vimeoIcon); break;
		}
		if (!data->title.isEmpty() || !data->duration.isEmpty()) {
			p.fillRect(skipx, skipy, width, st::msgDateFont->height + 2 * st::msgDateImgPadding.y(), st::msgDateImgBg->b);
			p.setFont(st::msgDateFont->f);
			p.setPen(st::msgDateImgColor->p);
			int32 titleWidth = width - 2 * st::msgDateImgPadding.x();
			if (!data->duration.isEmpty()) {
				int32 durationWidth = st::msgDateFont->m.width(data->duration);
				p.drawText(skipx + width - st::msgDateImgPadding.x() - durationWidth, skipy + st::msgDateImgPadding.y() + st::msgDateFont->ascent, data->duration);
				titleWidth -= durationWidth + st::msgDateImgPadding.x();
			}
			if (!data->title.isEmpty()) {
				p.drawText(skipx + st::msgDateImgPadding.x(), skipy + st::msgDateImgPadding.y() + st::msgDateFont->ascent, st::msgDateFont->m.elidedText(data->title, Qt::ElideRight, titleWidth));
			}
		}
	}
	if (selected) {
		App::roundRect(p, skipx, skipy, width, height, textstyleCurrent()->selectOverlay, SelectedOverlayCorners);
	}

	// date
	QString time(parent->time());
	if (time.isEmpty()) return;
	int32 dateX = skipx + width - parent->timeWidth(false) - st::msgDateImgDelta - 2 * st::msgDateImgPadding.x();
	int32 dateY = skipy + height - st::msgDateFont->height - 2 * st::msgDateImgPadding.y() - st::msgDateImgDelta;
	if (parent->out()) {
		dateX -= st::msgCheckRect.pxWidth() + st::msgDateImgCheckSpace;
	}
	int32 dateW = skipx + width - dateX - st::msgDateImgDelta;
	int32 dateH = skipy + height - dateY - st::msgDateImgDelta;

	App::roundRect(p, dateX, dateY, dateW, dateH, selected ? st::msgDateImgSelectBg : st::msgDateImgBg, selected ? DateSelectedCorners : DateCorners);

	p.setFont(st::msgDateFont->f);
	p.setPen(st::msgDateImgColor->p);
	p.drawText(dateX + st::msgDateImgPadding.x(), dateY + st::msgDateImgPadding.y() + st::msgDateFont->ascent, time);
	if (out) {
		QPoint iconPos(dateX - 2 + dateW - st::msgDateImgCheckSpace - st::msgCheckRect.pxWidth(), dateY + (dateH - st::msgCheckRect.pxHeight()) / 2);
		const QRect *iconRect;
		if (parent->id > 0) {
			if (parent->unread()) {
				iconRect = &st::msgImgCheckRect;
			} else {
				iconRect = &st::msgImgDblCheckRect;
			}
		} else {
			iconRect = &st::msgImgSendingRect;
		}
		p.drawPixmap(iconPos, App::sprite(), *iconRect);
	}
}

int32 HistoryImageLink::resize(int32 width, bool dontRecountText, const HistoryItem *parent) {
	const HistoryReply *reply = toHistoryReply(parent);
	const HistoryForwarded *fwd = toHistoryForwarded(parent);

	w = qMin(width, _maxw);
	if (reply || !_title.isEmpty() || !_description.isEmpty()) {
		w -= st::mediaPadding.left() + st::mediaPadding.right();
	}

	int32 tw = convertScale(fullWidth()), th = convertScale(fullHeight());
	if (tw > st::maxMediaSize) {
		th = (st::maxMediaSize * th) / tw;
		tw = st::maxMediaSize;
	}
	_height = th;
	if (tw > w) {
		_height = (w * _height / tw);
	} else {
		w = tw;
	}
	if (_height > width) {
		w = (w * width) / _height;
		_height = width;
	}
	if (w < st::minPhotoSize) {
		w = st::minPhotoSize;
	}
	if (_height < st::minPhotoSize) {
		_height = st::minPhotoSize;
	}
	if (reply || !_title.isEmpty() || !_description.isEmpty()) {
		if (!parent->out() && parent->history()->peer->chat) {
			_height += st::msgPadding.top() + st::msgNameFont->height;
		}
		if (reply) {
			_height += st::msgReplyPadding.top() + st::msgReplyBarSize.height() + st::msgReplyPadding.bottom();
		} else {
			if (parent->out() || !parent->history()->peer->chat || !fwd) {
				_height += st::msgPadding.top();
			}
			if (fwd) {
				_height += st::msgServiceNameFont->height + st::msgPadding.top();
			}
		}
		if (!_title.isEmpty()) {
			_height += qMin(_title.countHeight(w), st::webPageTitleFont->height * 2);
		}
		if (!_description.isEmpty()) {
			_height += qMin(_description.countHeight(w), st::webPageDescriptionFont->height * 3);
		}
		if (!_title.isEmpty() || !_description.isEmpty()) {
			_height += st::webPagePhotoSkip;
		}
		_height += st::mediaPadding.bottom();
		w += st::mediaPadding.left() + st::mediaPadding.right();
	}
	return _height;
}

const QString HistoryImageLink::inDialogsText() const {
	if (data) {
		switch (data->type) {
		case YouTubeLink: return qsl("YouTube Video");
		case VimeoLink: return qsl("Vimeo Video");
		case InstagramLink: return qsl("Instagram Link");
		case GoogleMapsLink: return lang(lng_maps_point);
		}
	}
	return QString();
}

const QString HistoryImageLink::inHistoryText() const {
	if (data) {
		switch (data->type) {
		case YouTubeLink: return qsl("[ YouTube Video : ") + link->text() + qsl(" ]");
		case VimeoLink: return qsl("[ Vimeo Video : ") + link->text() + qsl(" ]");
		case InstagramLink: return qsl("[ Instagram Link : ") + link->text() + qsl(" ]");
		case GoogleMapsLink: return qsl("[ ") + lang(lng_maps_point) + qsl(" : ") + link->text() + qsl(" ]");
		}
	}
	return qsl("[ Link : ") + link->text() + qsl(" ]");
}

bool HistoryImageLink::hasPoint(int32 x, int32 y, const HistoryItem *parent, int32 width) const {
	if (width < 0) width = w;
	return (x >= 0 && y >= 0 && x < width && y < _height);
}

void HistoryImageLink::getState(TextLinkPtr &lnk, HistoryCursorState &state, int32 x, int32 y, const HistoryItem *parent, int32 width) const {
	if (width < 0) width = w;
	int skipx = 0, skipy = 0, height = _height;
	const HistoryReply *reply = toHistoryReply(parent);
	const HistoryForwarded *fwd = reply ? 0 : toHistoryForwarded(parent);
	int replyFrom = 0, fwdFrom = 0;
	if (reply || !_title.isEmpty() || !_description.isEmpty()) {
		skipx = st::mediaPadding.left();
		if (reply) {
			skipy = st::msgReplyPadding.top() + st::msgReplyBarSize.height() + st::msgReplyPadding.bottom();
		} if (fwd) {
			skipy = st::msgServiceNameFont->height + st::msgPadding.top();
		}
		if (!parent->out() && parent->history()->peer->chat) {
			replyFrom = st::msgPadding.top() + st::msgNameFont->height;
			fwdFrom = st::msgPadding.top() + st::msgNameFont->height;
			skipy += replyFrom;
			if (x >= st::mediaPadding.left() && y >= st::msgPadding.top() && x < width - st::mediaPadding.left() - st::mediaPadding.right() && x < st::mediaPadding.left() + parent->from()->nameText.maxWidth() && y < replyFrom) {
				lnk = parent->from()->lnk;
				return;
			}
			if (!fwd && !reply) skipy += st::msgPadding.top();
		} else if (!reply) {
			fwdFrom = st::msgPadding.top();
			skipy += fwdFrom;
		}
		if (reply) {
			if (x >= 0 && y >= replyFrom + st::msgReplyPadding.top() && x < width && y < skipy - st::msgReplyPadding.bottom()) {
				lnk = reply->replyToLink();
				return;
			}
		} else if (fwd) {
			if (y >= fwdFrom && y < fwdFrom + st::msgServiceNameFont->height) {
				return fwd->getForwardedState(lnk, state, x - st::mediaPadding.left(), width - st::mediaPadding.left() - st::mediaPadding.right());
			}
		}
		height -= skipy + st::mediaPadding.bottom();
		width -= st::mediaPadding.left() + st::mediaPadding.right();
	}
	if (x >= skipx && y >= skipy && x < skipx + width && y < skipy + height && data) {
		lnk = link;

		int32 dateX = skipx + width - parent->timeWidth(false) - st::msgDateImgDelta - st::msgDateImgPadding.x();
		int32 dateY = skipy + height - st::msgDateFont->height - st::msgDateImgDelta - st::msgDateImgPadding.y();
		if (parent->out()) {
			dateX -= st::msgCheckRect.pxWidth() + st::msgDateImgCheckSpace;
		}
		bool inDate = QRect(dateX, dateY, parent->timeWidth(true) - st::msgDateSpace, st::msgDateFont->height).contains(x, y);
		if (inDate) {
			state = HistoryInDateCursorState;
		}

		return;
	}
}

HistoryMedia *HistoryImageLink::clone() const {
	return new HistoryImageLink(*this);
}

HistoryMessage::HistoryMessage(History *history, HistoryBlock *block, const MTPDmessage &msg) :
	HistoryItem(history, block, msg.vid.v, msg.vflags.v, ::date(msg.vdate), msg.vfrom_id.v)
, _text(st::msgMinWidth)
, _textWidth(0)
, _textHeight(0)
, _media(0)
{
	QString text(textClean(qs(msg.vmessage)));
	initTime();
	initMedia(msg.has_media() ? (&msg.vmedia) : 0, text);
	setText(text, msg.has_entities() ? linksFromMTP(msg.ventities.c_vector().v) : LinksInText());
}

HistoryMessage::HistoryMessage(History *history, HistoryBlock *block, MsgId msgId, int32 flags, QDateTime date, int32 from, const QString &msg, const LinksInText &links, const MTPMessageMedia *media) :
HistoryItem(history, block, msgId, flags, date, from)
, _text(st::msgMinWidth)
, _textWidth(0)
, _textHeight(0)
, _media(0)
{
	QString text(msg);
	initTime();
	initMedia(media, text);
	setText(text, links);
}

HistoryMessage::HistoryMessage(History *history, HistoryBlock *block, MsgId msgId, int32 flags, QDateTime date, int32 from, const QString &msg, const LinksInText &links, HistoryMedia *fromMedia) :
HistoryItem(history, block, msgId, flags, date, from)
, _text(st::msgMinWidth)
, _textWidth(0)
, _textHeight(0)
, _media(0)
{
	initTime();
	if (fromMedia) {
		_media = fromMedia->clone();
		_media->regItem(this);
	}
	setText(msg, links);
}

HistoryMessage::HistoryMessage(History *history, HistoryBlock *block, MsgId msgId, int32 flags, QDateTime date, int32 from, DocumentData *doc) :
HistoryItem(history, block, msgId, flags, date, from)
, _text(st::msgMinWidth)
, _textWidth(0)
, _textHeight(0)
, _media(0)
{
	initTime();
	initMediaFromDocument(doc);
	setText(QString(), LinksInText());
}

void HistoryMessage::initTime() {
	_time = date.toString(cTimeFormat());
	_timeWidth = st::msgDateFont->m.width(_time);
}

void HistoryMessage::initMedia(const MTPMessageMedia *media, QString &currentText) {
	switch (media ? media->type() : mtpc_messageMediaEmpty) {
	case mtpc_messageMediaContact: {
		const MTPDmessageMediaContact &d(media->c_messageMediaContact());
		_media = new HistoryContact(d.vuser_id.v, qs(d.vfirst_name), qs(d.vlast_name), qs(d.vphone_number));
	} break;
	case mtpc_messageMediaGeo: {
		const MTPGeoPoint &point(media->c_messageMediaGeo().vgeo);
		if (point.type() == mtpc_geoPoint) {
			const MTPDgeoPoint &d(point.c_geoPoint());
			_media = new HistoryImageLink(qsl("location:%1,%2").arg(d.vlat.v).arg(d.vlong.v));
		}
	} break;
	case mtpc_messageMediaVenue: {
		const MTPDmessageMediaVenue &d(media->c_messageMediaVenue());
		if (d.vgeo.type() == mtpc_geoPoint) {
			const MTPDgeoPoint &g(d.vgeo.c_geoPoint());
			_media = new HistoryImageLink(qsl("location:%1,%2").arg(g.vlat.v).arg(g.vlong.v), qs(d.vtitle), qs(d.vaddress));
		}
	} break;
	case mtpc_messageMediaPhoto: {
		const MTPDmessageMediaPhoto &photo(media->c_messageMediaPhoto());
		if (photo.vphoto.type() == mtpc_photo) {
			_media = new HistoryPhoto(photo.vphoto.c_photo(), qs(photo.vcaption), this);
		}
	} break;
	case mtpc_messageMediaVideo: {
		const MTPDmessageMediaVideo &video(media->c_messageMediaVideo());
		if (video.vvideo.type() == mtpc_video) {
			_media = new HistoryVideo(video.vvideo.c_video(), qs(video.vcaption), this);
		}
	} break;
	case mtpc_messageMediaAudio: {
		const MTPAudio &audio(media->c_messageMediaAudio().vaudio);
		if (audio.type() == mtpc_audio) {
			_media = new HistoryAudio(audio.c_audio());
		}
	} break;
	case mtpc_messageMediaDocument: {
		const MTPDocument &document(media->c_messageMediaDocument().vdocument);
		if (document.type() == mtpc_document) {
			DocumentData *doc = App::feedDocument(document);
			return initMediaFromDocument(doc);
		}
	} break;
	case mtpc_messageMediaWebPage: {
		const MTPWebPage &d(media->c_messageMediaWebPage().vwebpage);
		switch (d.type()) {
		case mtpc_webPageEmpty: initMediaFromText(currentText); break;
		case mtpc_webPagePending: {
			WebPageData *webPage = App::feedWebPage(d.c_webPagePending());
			_media = new HistoryWebPage(webPage);
		} break;
		case mtpc_webPage: {
			_media = new HistoryWebPage(App::feedWebPage(d.c_webPage()));
		} break;
		}
	} break;
	default: initMediaFromText(currentText); break;
	};
	if (_media) _media->regItem(this);
}

void HistoryMessage::initMediaFromText(QString &currentText)  {
	QString lnk = currentText.trimmed();
	if (/*reYouTube1.match(currentText).hasMatch() || reYouTube2.match(currentText).hasMatch() || reInstagram.match(currentText).hasMatch() || */reVimeo.match(currentText).hasMatch()) {
		_media = new HistoryImageLink(lnk);
		currentText = QString();
	}
}

void HistoryMessage::initMediaFromDocument(DocumentData *doc) {
	if (doc->sticker()) {
		_media = new HistorySticker(doc);
	} else {
		_media = new HistoryDocument(doc);
	}
	_media->regItem(this);
}

void HistoryMessage::initDimensions(const HistoryItem *parent) {
	if (justMedia()) {
		_media->initDimensions(this);
		_maxw = _media->maxWidth();
		_minh = _media->minHeight();
	} else {
		_maxw = _text.maxWidth();
		_minh = _text.minHeight();
		_maxw += st::msgPadding.left() + st::msgPadding.right();
		if (_media) {
			_media->initDimensions(this);
			if (_media->isDisplayed() && _text.hasSkipBlock()) {
				_text.removeSkipBlock();
				_textWidth = 0;
				_textHeight = 0;
			} else if (!_media->isDisplayed() && !_text.hasSkipBlock()) {
				_text.setSkipBlock(timeWidth(true), st::msgDateFont->height - st::msgDateDelta.y());
				_textWidth = 0;
				_textHeight = 0;
			}
			if (_media->isDisplayed()) {
				int32 maxw = _media->maxWidth() + st::msgPadding.left() + st::msgPadding.right();
				if (maxw > _maxw) _maxw = maxw;
				_minh += st::msgPadding.bottom() + _media->minHeight();
			}
		}
	}
	fromNameUpdated();
}

void HistoryMessage::fromNameUpdated() const {
	if (_media) return;
	int32 _namew = ((!out() && _history->peer->chat) ? _from->nameText.maxWidth() : 0) + st::msgPadding.left() + st::msgPadding.right();
	if (_namew > _maxw) _maxw = _namew;
}

bool HistoryMessage::uploading() const {
	return _media ? _media->uploading() : false;
}

QString HistoryMessage::selectedText(uint32 selection) const {
	if (_media && selection == FullItemSel) {
		QString text = _text.original(0, 0xFFFF), mediaText = _media->inHistoryText();
		return text.isEmpty() ? mediaText : (mediaText.isEmpty() ? text : (text + ' ' + mediaText));
	}
	uint16 selectedFrom = (selection == FullItemSel) ? 0 : (selection >> 16) & 0xFFFF;
	uint16 selectedTo = (selection == FullItemSel) ? 0xFFFF : (selection & 0xFFFF);
	return _text.original(selectedFrom, selectedTo);
}

LinksInText HistoryMessage::textLinks() const {
	return _text.calcLinksInText();
}

QString HistoryMessage::inDialogsText() const {
	QString result = _media ? _media->inDialogsText() : QString();
	return result.isEmpty() ? _text.original(0, 0xFFFF, false) : result;
}

HistoryMedia *HistoryMessage::getMedia(bool inOverview) const {
	return _media;
}

void HistoryMessage::setMedia(const MTPMessageMedia *media) {
	if ((!_media || _media->isImageLink()) && (!media || media->type() == mtpc_messageMediaEmpty)) return;

	bool mediaWasDisplayed = false;
	if (_media) {
		mediaWasDisplayed = _media->isDisplayed();
		delete _media;
		_media = 0;
	}
	QString t;
	initMedia(media, t);
	if (_media && _media->isDisplayed() && !mediaWasDisplayed) {
		_text.removeSkipBlock();
		_textWidth = 0;
		_textHeight = 0;
	} else if (mediaWasDisplayed && (!_media || !_media->isDisplayed())) {
		_text.setSkipBlock(timeWidth(true), st::msgDateFont->height - st::msgDateDelta.y());
		_textWidth = 0;
		_textHeight = 0;
	}
	initDimensions(0);
	if (App::main()) App::main()->itemResized(this);
}

void HistoryMessage::setText(const QString &text, const LinksInText &links) {
	if (!_media || !text.isEmpty()) { // !justMedia()
		if (_media && _media->isDisplayed()) {
			_text.setMarkedText(st::msgFont, text, links, itemTextParseOptions(this));
		} else {
			_text.setMarkedText(st::msgFont, text + textcmdSkipBlock(timeWidth(true), st::msgDateFont->height - st::msgDateDelta.y()), links, itemTextParseOptions(this));
		}
		if (id > 0) {
			for (int32 i = 0, l = links.size(); i != l; ++i) {
				if (links.at(i).type == LinkInTextUrl || links.at(i).type == LinkInTextCustomUrl || links.at(i).type == LinkInTextEmail) {
					_flags |= MTPDmessage_flag_HAS_TEXT_LINKS;
					break;
				}
			}
		}
		_textWidth = 0;
		_textHeight = 0;
	}
}

void HistoryMessage::getTextWithLinks(QString &text, LinksInText &links) {
	if (_text.isEmpty()) return;
	links = _text.calcLinksInText();
	text = _text.original();
}

bool HistoryMessage::textHasLinks() {
	return _text.hasLinks();
}

void HistoryMessage::draw(QPainter &p, uint32 selection) const {
	textstyleSet(&(out() ? st::outTextStyle : st::inTextStyle));

	uint64 ms = App::main() ? App::main()->animActiveTime(id) : 0;
	if (ms) {
		if (ms > st::activeFadeInDuration + st::activeFadeOutDuration) {
			App::main()->stopAnimActive();
		} else {
			float64 dt = (ms > st::activeFadeInDuration) ? (1 - (ms - st::activeFadeInDuration) / float64(st::activeFadeOutDuration)) : (ms / float64(st::activeFadeInDuration));
			float64 o = p.opacity();
			p.setOpacity(o * dt);
			p.fillRect(0, 0, _history->width, _height, textstyleCurrent()->selectOverlay->b);
			p.setOpacity(o);
		}
	}

	bool selected = (selection == FullItemSel);
	if (_from->nameVersion > _fromVersion) {
		fromNameUpdated();
		_fromVersion = _from->nameVersion;
	}
	int32 left = out() ? st::msgMargin.right() : st::msgMargin.left(), width = _history->width - st::msgMargin.left() - st::msgMargin.right(), mwidth = st::msgMaxWidth;
	if (justMedia()) {
		if (_media->maxWidth() > mwidth) mwidth = _media->maxWidth();
		if (_media->currentWidth() < mwidth) mwidth = _media->currentWidth();
	}
	if (width > mwidth) {
		if (out()) left += width - mwidth;
		width = mwidth;
	}

	if (!out() && _history->peer->chat) {
		p.drawPixmap(left, _height - st::msgMargin.bottom() - st::msgPhotoSize, _from->photo->pixRounded(st::msgPhotoSize));
//		width -= st::msgPhotoSkip;
		left += st::msgPhotoSkip;
	}
	if (width < 1) return;

	if (width >= _maxw) {
		if (out()) left += width - _maxw;
		width = _maxw;
	}
	if (justMedia()) {
		p.save();
		p.translate(left, st::msgMargin.top());
		_media->draw(p, this, selected);
		p.restore();
	} else {
		QRect r(left, st::msgMargin.top(), width, _height - st::msgMargin.top() - st::msgMargin.bottom());

		style::color bg(selected ? (out() ? st::msgOutSelectBg : st::msgInSelectBg) : (out() ? st::msgOutBg : st::msgInBg));
		style::color sh(selected ? (out() ? st::msgOutSelectShadow : st::msgInSelectShadow) : (out() ? st::msgOutShadow : st::msgInShadow));
		RoundCorners cors(selected ? (out() ? MessageOutSelectedCorners : MessageInSelectedCorners) : (out() ? MessageOutCorners : MessageInCorners));
		App::roundRect(p, r, bg, cors, &sh);

		if (!out() && _history->peer->chat) {
			p.setFont(st::msgNameFont->f);
			p.setPen(_from->color->p);
			_from->nameText.drawElided(p, r.left() + st::msgPadding.left(), r.top() + st::msgPadding.top(), width - st::msgPadding.left() - st::msgPadding.right());
			r.setTop(r.top() + st::msgNameFont->height);
		}
		QRect trect(r.marginsAdded(-st::msgPadding));
		drawMessageText(p, trect, selection);

		if (_media) {
			p.save();
			p.translate(left + st::msgPadding.left(), _height - st::msgMargin.bottom() - st::msgPadding.bottom() - _media->height());
			_media->draw(p, this, selected);
			p.restore();
		}
		p.setFont(st::msgDateFont->f);

		style::color date(selected ? (out() ? st::msgOutSelectDateColor : st::msgInSelectDateColor) : (out() ? st::msgOutDateColor : st::msgInDateColor));
		p.setPen(date->p);

		p.drawText(r.right() - st::msgPadding.right() + st::msgDateDelta.x() - timeWidth(true) + st::msgDateSpace, r.bottom() - st::msgPadding.bottom() + st::msgDateDelta.y() - st::msgDateFont->descent, _time);
		if (out()) {
			QPoint iconPos(r.right() + st::msgCheckPos.x() - st::msgPadding.right() - st::msgCheckRect.pxWidth(), r.bottom() + st::msgCheckPos.y() - st::msgPadding.bottom() + st::msgDateDelta.y() - st::msgCheckRect.pxHeight());
			const QRect *iconRect;
			if (id > 0) {
				if (unread()) {
					iconRect = &(selected ? st::msgSelectCheckRect : st::msgCheckRect);
				} else {
					iconRect = &(selected ? st::msgSelectDblCheckRect : st::msgDblCheckRect);
				}
			} else {
				iconRect = &st::msgSendingRect;
			}
			p.drawPixmap(iconPos, App::sprite(), *iconRect);
		}
	}
}

void HistoryMessage::drawMessageText(QPainter &p, const QRect &trect, uint32 selection) const {
	p.setPen(st::msgColor->p);
	p.setFont(st::msgFont->f);
	uint16 selectedFrom = (selection == FullItemSel) ? 0 : (selection >> 16) & 0xFFFF;
	uint16 selectedTo = (selection == FullItemSel) ? 0 : selection & 0xFFFF;
	_text.draw(p, trect.x(), trect.y(), trect.width(), Qt::AlignLeft, 0, -1, selectedFrom, selectedTo);

	textstyleRestore();
}

int32 HistoryMessage::resize(int32 width, bool dontRecountText, const HistoryItem *parent) {
	if (width < st::msgMinWidth) return _height;

	width -= st::msgMargin.left() + st::msgMargin.right();
	if (justMedia()) {
		_height = _media->resize(width, dontRecountText, this);
	} else {
		if (dontRecountText && !_media) return _height;

		if (width < st::msgPadding.left() + st::msgPadding.right() + 1) {
			width = st::msgPadding.left() + st::msgPadding.right() + 1;
		} else if (width > st::msgMaxWidth) {
			width = st::msgMaxWidth;
		}
		int32 nwidth = qMax(width - st::msgPadding.left() - st::msgPadding.right(), 0);
		if (nwidth != _textWidth) {
			_textWidth = nwidth;
			_textHeight = _text.countHeight(nwidth);
		}
		if (width >= _maxw) {
			_height = _minh;
			if (_media) _media->resize(_maxw - st::msgPadding.left() - st::msgPadding.right(), dontRecountText, this);
		} else {
			_height = _textHeight;
			if (_media && _media->isDisplayed()) _height += st::msgPadding.bottom() + _media->resize(nwidth, dontRecountText, this);
		}
		if (!out() && _history->peer->chat) {
			_height += st::msgNameFont->height;
		}
		_height += st::msgPadding.top() + st::msgPadding.bottom();
	}
	_height += st::msgMargin.top() + st::msgMargin.bottom();
	return _height;
}

bool HistoryMessage::hasPoint(int32 x, int32 y) const {
	int32 left = out() ? st::msgMargin.right() : st::msgMargin.left(), width = _history->width - st::msgMargin.left() - st::msgMargin.right(), mwidth = st::msgMaxWidth;
	if (justMedia()) {
		if (_media->maxWidth() > mwidth) mwidth = _media->maxWidth();
		if (_media->currentWidth() < mwidth) mwidth = _media->currentWidth();
	}
	if (width > mwidth) {
		if (out()) left += width - mwidth;
		width = mwidth;
	}

	if (!out() && _history->peer->chat) { // from user left photo
		left += st::msgPhotoSkip;
	}
	if (width < 1) return false;

	if (width >= _maxw) {
		if (out()) left += width - _maxw;
		width = _maxw;
	}
	if (justMedia()) {
		return _media->hasPoint(x - left, y - st::msgMargin.top(), this);
	}
	QRect r(left, st::msgMargin.top(), width, _height - st::msgMargin.top() - st::msgMargin.bottom());
	return r.contains(x, y);
}

void HistoryMessage::getState(TextLinkPtr &lnk, HistoryCursorState &state, int32 x, int32 y) const {
	state = HistoryDefaultCursorState;
	lnk = TextLinkPtr();

	int32 left = out() ? st::msgMargin.right() : st::msgMargin.left(), width = _history->width - st::msgMargin.left() - st::msgMargin.right(), mwidth = st::msgMaxWidth;
	if (justMedia()) {
		if (_media->maxWidth() > mwidth) mwidth = _media->maxWidth();
		if (_media->currentWidth() < mwidth) mwidth = _media->currentWidth();
	}
	if (width > mwidth) {
		if (out()) left += width - mwidth;
		width = mwidth;
	}

	if (!out() && _history->peer->chat) { // from user left photo
		if (x >= left && x < left + st::msgPhotoSize && y >= _height - st::msgMargin.bottom() - st::msgPhotoSize && y < _height - st::msgMargin.bottom()) {
			lnk = _from->lnk;
			return;
		}
		//		width -= st::msgPhotoSkip;
		left += st::msgPhotoSkip;
	}
	if (width < 1) return;

	if (width >= _maxw) {
		if (out()) left += width - _maxw;
		width = _maxw;
	}
	if (justMedia()) {
		_media->getState(lnk, state, x - left, y - st::msgMargin.top(), this);
		return;
	}
	QRect r(left, st::msgMargin.top(), width, _height - st::msgMargin.top() - st::msgMargin.bottom());
	if (!out() && _history->peer->chat) { // from user left name
		if (x >= r.left() + st::msgPadding.left() && y >= r.top() + st::msgPadding.top() && y < r.top() + st::msgPadding.top() + st::msgNameFont->height && x < r.left() + r.width() - st::msgPadding.right() && x < r.left() + st::msgPadding.left() + _from->nameText.maxWidth()) {
			lnk = _from->lnk;
			return;
		}
		r.setTop(r.top() + st::msgNameFont->height);
	}

	getStateFromMessageText(lnk, state, x, y, r);
}

void HistoryMessage::getStateFromMessageText(TextLinkPtr &lnk, HistoryCursorState &state, int32 x, int32 y, const QRect &r) const {
	int32 dateX = r.right() - st::msgPadding.right() + st::msgDateDelta.x() - timeWidth(true) + st::msgDateSpace;
	int32 dateY = r.bottom() - st::msgPadding.bottom() + st::msgDateDelta.y() - st::msgDateFont->height;
	bool inDate = QRect(dateX, dateY, timeWidth(true) - st::msgDateSpace, st::msgDateFont->height).contains(x, y);

	QRect trect(r.marginsAdded(-st::msgPadding));
	TextLinkPtr medialnk;
	if (_media && _media->isDisplayed()) {
		if (y >= trect.bottom() - _media->height() && y < trect.bottom()) {
			_media->getState(lnk, state, x - trect.left(), y + _media->height() - trect.bottom(), this);
			if (inDate) state = HistoryInDateCursorState;
			return;
		}
		trect.setBottom(trect.bottom() - _media->height() - st::msgPadding.bottom());
	}
	bool inText = false;
	_text.getState(lnk, inText, x - trect.x(), y - trect.y(), trect.width());

	if (inDate) {
		state = HistoryInDateCursorState;
	} else if (inText) {
		state = HistoryInTextCursorState;
	} else {
		state = HistoryDefaultCursorState;
	}
}

void HistoryMessage::getSymbol(uint16 &symbol, bool &after, bool &upon, int32 x, int32 y) const {
	symbol = 0;
	after = false;
	upon = false;
	if (justMedia()) return;

	int32 left = out() ? st::msgMargin.right() : st::msgMargin.left(), width = _history->width - st::msgMargin.left() - st::msgMargin.right();
	if (width > st::msgMaxWidth) {
		if (out()) left += width - st::msgMaxWidth;
		width = st::msgMaxWidth;
	}

	if (!out() && _history->peer->chat) { // from user left photo
//		width -= st::msgPhotoSkip;
		left += st::msgPhotoSkip;
	}
	if (width < 1) return;

	if (width >= _maxw) {
		if (out()) left += width - _maxw;
		width = _maxw;
	}
	QRect r(left, st::msgMargin.top(), width, _height - st::msgMargin.top() - st::msgMargin.bottom());
	if (!out() && _history->peer->chat) { // from user left name
		r.setTop(r.top() + st::msgNameFont->height);
	}
	QRect trect(r.marginsAdded(-st::msgPadding));
	if (_media && _media->isDisplayed()) {
		trect.setBottom(trect.bottom() - _media->height() - st::msgPadding.bottom());
	}
	_text.getSymbol(symbol, after, upon, x - trect.x(), y - trect.y(), trect.width());
}

void HistoryMessage::drawInDialog(QPainter &p, const QRect &r, bool act, const HistoryItem *&cacheFor, Text &cache) const {
	if (cacheFor != this) {
		cacheFor = this;
		QString msg(inDialogsText());
		if (_history->peer->chat || out()) {
			TextCustomTagsMap custom;
			custom.insert(QChar('c'), qMakePair(textcmdStartLink(1), textcmdStopLink()));
			msg = lng_message_with_from(lt_from, textRichPrepare((_from == App::self()) ? lang(lng_from_you) : _from->firstName), lt_message, textRichPrepare(msg));
			cache.setRichText(st::dlgHistFont, msg, _textDlgOptions, custom);
		} else {
			cache.setText(st::dlgHistFont, msg, _textDlgOptions);
		}
	}
	if (r.width()) {
		textstyleSet(&(act ? st::dlgActiveTextStyle : st::dlgTextStyle));
		p.setFont(st::dlgHistFont->f);
		p.setPen((act ? st::dlgActiveColor : (justMedia() ? st::dlgSystemColor : st::dlgTextColor))->p);
		cache.drawElided(p, r.left(), r.top(), r.width(), r.height() / st::dlgHistFont->height);
		textstyleRestore();
	}
}

QString HistoryMessage::notificationHeader() const {
    return _history->peer->chat ? from()->name : QString();
}

QString HistoryMessage::notificationText() const {
	QString msg(inDialogsText());
    if (msg.size() > 0xFF) msg = msg.mid(0, 0xFF) + qsl("..");
    return msg;
}

HistoryMessage::~HistoryMessage() {
	if (_media) {
		_media->unregItem(this);
		delete _media;
	}
	if (_flags & MTPDmessage::flag_reply_markup) {
		App::clearReplyMarkup(id);
	}
}

HistoryForwarded::HistoryForwarded(History *history, HistoryBlock *block, const MTPDmessage &msg) : HistoryMessage(history, block, msg.vid.v, msg.vflags.v, ::date(msg.vdate), msg.vfrom_id.v, textClean(qs(msg.vmessage)), msg.has_entities() ? linksFromMTP(msg.ventities.c_vector().v) : LinksInText(), msg.has_media() ? (&msg.vmedia) : 0)
, fwdDate(::date(msg.vfwd_date))
, fwdFrom(App::user(msg.vfwd_from_id.v))
, fwdFromVersion(fwdFrom->nameVersion)
, fromWidth(st::msgServiceFont->m.width(lang(lng_forwarded_from)) + st::msgServiceFont->spacew)
{
	fwdNameUpdated();
}

HistoryForwarded::HistoryForwarded(History *history, HistoryBlock *block, MsgId id, HistoryMessage *msg) : HistoryMessage(history, block, id, ((history->peer->input.type() != mtpc_inputPeerSelf) ? (MTPDmessage_flag_out | MTPDmessage_flag_unread) : 0) | (msg->getMedia() && (msg->getMedia()->type() == MediaTypeAudio/* || msg->getMedia()->type() == MediaTypeVideo*/) ? MTPDmessage_flag_media_unread : 0), ::date(unixtime()), MTP::authedId(), msg->justMedia() ? QString() : msg->HistoryMessage::selectedText(FullItemSel), msg->HistoryMessage::textLinks(), msg->getMedia())
, fwdDate(msg->dateForwarded())
, fwdFrom(msg->fromForwarded())
, fwdFromVersion(fwdFrom->nameVersion)
, fromWidth(st::msgServiceFont->m.width(lang(lng_forwarded_from)) + st::msgServiceFont->spacew)
{
	fwdNameUpdated();
}

QString HistoryForwarded::selectedText(uint32 selection) const {
	if (selection != FullItemSel) return HistoryMessage::selectedText(selection);
	QString result, original = HistoryMessage::selectedText(selection);
	result.reserve(lang(lng_forwarded_from).size() + fwdFrom->name.size() + 4 + original.size());
	result.append('[').append(lang(lng_forwarded_from)).append(' ').append(fwdFrom->name).append(qsl("]\n")).append(original);
	return result;
}

void HistoryForwarded::initDimensions(const HistoryItem *parent) {
	HistoryMessage::initDimensions(parent);
	fwdNameUpdated();
}

void HistoryForwarded::fwdNameUpdated() const {
	fwdFromName.setText(st::msgServiceNameFont, App::peerName(fwdFrom), _textNameOptions);
	if (justMedia()) return;
	int32 _namew = fromWidth + fwdFromName.maxWidth() + st::msgPadding.left() + st::msgPadding.right();
	if (_namew > _maxw) _maxw = _namew;
}

void HistoryForwarded::draw(QPainter &p, uint32 selection) const {
	if (!justMedia() && fwdFrom->nameVersion > fwdFromVersion) {
		fwdNameUpdated();
		fwdFromVersion = fwdFrom->nameVersion;
	}
	HistoryMessage::draw(p, selection);
}

void HistoryForwarded::drawForwardedFrom(QPainter &p, int32 x, int32 y, int32 w, bool selected) const {
	style::font serviceFont(st::msgServiceFont), serviceName(st::msgServiceNameFont);

	p.setPen((selected ? (out() ? st::msgOutServiceSelColor : st::msgInServiceSelColor) : (out() ? st::msgOutServiceColor : st::msgInServiceColor))->p);
	p.setFont(serviceFont->f);

	if (w >= fromWidth) {
		p.drawText(x, y + serviceFont->ascent, lang(lng_forwarded_from));

		p.setFont(serviceName->f);
		fwdFromName.drawElided(p, x + fromWidth, y, w - fromWidth);
	} else {
		p.drawText(x, y + serviceFont->ascent, serviceFont->m.elidedText(lang(lng_forwarded_from), Qt::ElideRight, w));
	}
}

void HistoryForwarded::drawMessageText(QPainter &p, const QRect &trect, uint32 selection) const {
	drawForwardedFrom(p, trect.x(), trect.y(), trect.width(), (selection == FullItemSel));

	QRect realtrect(trect);
	realtrect.setY(trect.y() + st::msgServiceNameFont->height);
	HistoryMessage::drawMessageText(p, realtrect, selection);
}

int32 HistoryForwarded::resize(int32 width, bool dontRecountText, const HistoryItem *parent) {
	HistoryMessage::resize(width, dontRecountText, parent);
	if (!justMedia() && (_media || !dontRecountText)) {
		_height += st::msgServiceNameFont->height;
	}
	return _height;
}

bool HistoryForwarded::hasPoint(int32 x, int32 y) const {
	if (!justMedia()) {
		int32 left = out() ? st::msgMargin.right() : st::msgMargin.left(), width = _history->width - st::msgMargin.left() - st::msgMargin.right();
		if (width > st::msgMaxWidth) {
			if (out()) left += width - st::msgMaxWidth;
			width = st::msgMaxWidth;
		}

		if (!out() && _history->peer->chat) { // from user left photo
//			width -= st::msgPhotoSkip;
			left += st::msgPhotoSkip;
		}
		if (width < 1) return false;

		if (width >= _maxw) {
			if (out()) left += width - _maxw;
			width = _maxw;
		}
		QRect r(left, st::msgMargin.top(), width, _height - st::msgMargin.top() - st::msgMargin.bottom());
		return r.contains(x, y);
	}
	return HistoryMessage::hasPoint(x, y);
}

void HistoryForwarded::getState(TextLinkPtr &lnk, HistoryCursorState &state, int32 x, int32 y) const {
	lnk = TextLinkPtr();
	state = HistoryDefaultCursorState;

	if (!justMedia()) {
		int32 left = out() ? st::msgMargin.right() : st::msgMargin.left(), width = _history->width - st::msgMargin.left() - st::msgMargin.right();
		if (width > st::msgMaxWidth) {
			if (out()) left += width - st::msgMaxWidth;
			width = st::msgMaxWidth;
		}

		if (!out() && _history->peer->chat) { // from user left photo
			if (x >= left && x < left + st::msgPhotoSize) {
				return HistoryMessage::getState(lnk, state, x, y);
			}
//			width -= st::msgPhotoSkip;
			left += st::msgPhotoSkip;
		}
		if (width < 1) return;

		if (width >= _maxw) {
			if (out()) left += width - _maxw;
			width = _maxw;
		}
		QRect r(left, st::msgMargin.top(), width, _height - st::msgMargin.top() - st::msgMargin.bottom());
		if (!out() && _history->peer->chat) {
			style::font nameFont(st::msgNameFont);
			if (y >= r.top() + st::msgPadding.top() && y < r.top() + st::msgPadding.top() + nameFont->height) {
				return HistoryMessage::getState(lnk, state, x, y);
			}
			r.setTop(r.top() + nameFont->height);
		}
		QRect trect(r.marginsAdded(-st::msgPadding));

		if (y >= trect.top() && y < trect.top() + st::msgServiceNameFont->height) {
			return getForwardedState(lnk, state, x - trect.left(), trect.right() - trect.left());
		}
		y -= st::msgServiceNameFont->height;
	}
	return HistoryMessage::getState(lnk, state, x, y);
}

void HistoryForwarded::getStateFromMessageText(TextLinkPtr &lnk, HistoryCursorState &state, int32 x, int32 y, const QRect &r) const {
	QRect realr(r);
	realr.setHeight(r.height() - st::msgServiceNameFont->height);
	HistoryMessage::getStateFromMessageText(lnk, state, x, y, realr);
}

void HistoryForwarded::getForwardedState(TextLinkPtr &lnk, HistoryCursorState &state, int32 x, int32 w) const {
	state = HistoryDefaultCursorState;
	if (x >= fromWidth && x < w && x < fromWidth + fwdFromName.maxWidth()) {
		lnk = fwdFrom->lnk;
	} else {
		lnk = TextLinkPtr();
	}
}

void HistoryForwarded::getSymbol(uint16 &symbol, bool &after, bool &upon, int32 x, int32 y) const {
	symbol = 0;
	after = false;
	upon = false;

	if (!justMedia()) {
		int32 left = out() ? st::msgMargin.right() : st::msgMargin.left(), width = _history->width - st::msgMargin.left() - st::msgMargin.right();
		if (width > st::msgMaxWidth) {
			if (out()) left += width - st::msgMaxWidth;
			width = st::msgMaxWidth;
		}

		if (!out() && _history->peer->chat) { // from user left photo
//			width -= st::msgPhotoSkip;
			left += st::msgPhotoSkip;
		}
		if (width < 1) return;

		if (width >= _maxw) {
			if (out()) left += width - _maxw;
			width = _maxw;
		}
		QRect r(left, st::msgMargin.top(), width, _height - st::msgMargin.top() - st::msgMargin.bottom());
		if (!out() && _history->peer->chat) {
			style::font nameFont(st::msgNameFont);
			if (y >= r.top() + st::msgPadding.top() && y < r.top() + st::msgPadding.top() + nameFont->height) {
				return HistoryMessage::getSymbol(symbol, after, upon, x, y);
			}
			r.setTop(r.top() + nameFont->height);
		}
		QRect trect(r.marginsAdded(-st::msgPadding));

		y -= st::msgServiceNameFont->height;
	}
	return HistoryMessage::getSymbol(symbol, after, upon, x, y);
}

HistoryReply::HistoryReply(History *history, HistoryBlock *block, const MTPDmessage &msg) : HistoryMessage(history, block, msg.vid.v, msg.vflags.v, ::date(msg.vdate), msg.vfrom_id.v, textClean(qs(msg.vmessage)), msg.has_entities() ? linksFromMTP(msg.ventities.c_vector().v) : LinksInText(), msg.has_media() ? (&msg.vmedia) : 0)
, replyToMsgId(msg.vreply_to_msg_id.v)
, replyToMsg(0)
, replyToVersion(0)
, _maxReplyWidth(0)
{
	if (!updateReplyTo()) {
		App::api()->requestReplyTo(this, replyToMsgId);
	}
}

HistoryReply::HistoryReply(History *history, HistoryBlock *block, MsgId msgId, int32 flags, MsgId replyTo, QDateTime date, int32 from, DocumentData *doc) : HistoryMessage(history, block, msgId, flags, date, from, doc)
, replyToMsgId(replyTo)
, replyToMsg(0)
, replyToVersion(0)
, _maxReplyWidth(0)
{
	if (!updateReplyTo()) {
		App::api()->requestReplyTo(this, replyToMsgId);
	}
}

QString HistoryReply::selectedText(uint32 selection) const {
	if (selection != FullItemSel || !replyToMsg) return HistoryMessage::selectedText(selection);
	QString result, original = HistoryMessage::selectedText(selection);
	result.reserve(lang(lng_in_reply_to).size() + replyToMsg->from()->name.size() + 4 + original.size());
	result.append('[').append(lang(lng_in_reply_to)).append(' ').append(replyToMsg->from()->name).append(qsl("]\n")).append(original);
	return result;
}

void HistoryReply::initDimensions(const HistoryItem *parent) {
	if (!replyToMsg) {
		_maxReplyWidth = st::msgReplyBarSkip + st::msgDateFont->m.width(lang(replyToMsgId ? lng_profile_loading : lng_deleted_message)) + st::msgPadding.left() + st::msgPadding.right();
	}
	HistoryMessage::initDimensions(parent);
	if (replyToMsg) {
		replyToNameUpdated();
	} else if (!justMedia()) {
		int maxw = _maxReplyWidth - st::msgReplyPadding.left() - st::msgReplyPadding.right() + st::msgPadding.left() + st::msgPadding.right();
		if (maxw > _maxw) _maxw = maxw;
	}
}

bool HistoryReply::updateReplyTo(bool force) {
	if (replyToMsg || !replyToMsgId) return true;
	replyToMsg = App::histItemById(replyToMsgId);
	if (replyToMsg) {
		App::historyRegReply(this, replyToMsg);
		replyToText.setText(st::msgFont, replyToMsg->inReplyText(), _textDlgOptions);

		replyToNameUpdated();
		
		replyToLnk = TextLinkPtr(new MessageLink(replyToMsg->history()->peer->id, replyToMsg->id));
	} else if (force) {
		replyToMsgId = 0;
	}
	if (force) {
		initDimensions();
		if (App::main()) App::main()->msgUpdated(history()->peer->id, this);
	}
	return (replyToMsg || !replyToMsgId);
}

void HistoryReply::replyToNameUpdated() const {
	if (!replyToMsg) return;
	replyToName.setText(st::msgServiceNameFont, App::peerName(replyToMsg->from()), _textNameOptions);
	replyToVersion = replyToMsg->from()->nameVersion;
	bool hasPreview = replyToMsg->getMedia() ? replyToMsg->getMedia()->hasReplyPreview() : false;
	int previewSkip = hasPreview ? (st::msgReplyBarSize.height() + st::msgReplyBarSkip - st::msgReplyBarSize.width() - st::msgReplyBarPos.x()) : 0;

	_maxReplyWidth = st::msgReplyPadding.left() + st::msgReplyBarSkip + previewSkip + replyToName.maxWidth() + st::msgReplyPadding.right();
	int32 _textw = st::msgReplyPadding.left() + st::msgReplyBarSkip + previewSkip + qMin(replyToText.maxWidth(), 4 * replyToName.maxWidth()) + st::msgReplyPadding.right();
	if (_textw > _maxReplyWidth) _maxReplyWidth = _textw;
	if (!justMedia()) {
		int maxw = _maxReplyWidth - st::msgReplyPadding.left() - st::msgReplyPadding.right() + st::msgPadding.left() + st::msgPadding.right();
		if (maxw > _maxw) _maxw = maxw;
	}
}

int32 HistoryReply::replyToWidth() const {
	return _maxReplyWidth;
}

TextLinkPtr HistoryReply::replyToLink() const {
	return replyToLnk;
}

MsgId HistoryReply::replyToId() const {
	return replyToMsgId;
}

HistoryItem *HistoryReply::replyToMessage() const {
	return replyToMsg;
}

void HistoryReply::replyToReplaced(HistoryItem *oldItem, HistoryItem *newItem) {
	if (replyToMsg == oldItem) {
		replyToMsg = newItem;
		if (!newItem) {
			replyToMsgId = 0;
			initDimensions();
		}
	}
}

void HistoryReply::draw(QPainter &p, uint32 selection) const {
	if (replyToMsg && replyToMsg->from()->nameVersion > replyToVersion) {
		replyToNameUpdated();
	}
	HistoryMessage::draw(p, selection);
}

void HistoryReply::drawReplyTo(QPainter &p, int32 x, int32 y, int32 w, bool selected, bool likeService) const {
	style::color bar;
	if (likeService) {
		bar = st::white;
	} else {
		bar = (selected ? (out() ? st::msgOutReplyBarSelColor : st::msgInReplyBarSelColor) : (out() ? st::msgOutReplyBarColor : st::msgInReplyBarColor));
	}
	p.fillRect(x + st::msgReplyBarPos.x(), y + st::msgReplyPadding.top() + st::msgReplyBarPos.y(), st::msgReplyBarSize.width(), st::msgReplyBarSize.height(), bar->b);

	if (w > st::msgReplyBarSkip) {
		if (replyToMsg) {
			bool hasPreview = replyToMsg->getMedia() ? replyToMsg->getMedia()->hasReplyPreview() : false;
			int previewSkip = hasPreview ? (st::msgReplyBarSize.height() + st::msgReplyBarSkip - st::msgReplyBarSize.width() - st::msgReplyBarPos.x()) : 0;

			if (hasPreview) {
				ImagePtr replyPreview = replyToMsg->getMedia()->replyPreview();
				if (!replyPreview->isNull()) {
					QRect to(x + st::msgReplyBarSkip, y + st::msgReplyPadding.top() + st::msgReplyBarPos.y(), st::msgReplyBarSize.height(), st::msgReplyBarSize.height());
					p.drawPixmap(to.x(), to.y(), replyPreview->pixSingle(replyPreview->width() / cIntRetinaFactor(), replyPreview->height() / cIntRetinaFactor(), to.width(), to.height()));
					if (selected) {
						App::roundRect(p, to, textstyleCurrent()->selectOverlay, SelectedOverlayCorners);
					}
				}
			}
			if (w > st::msgReplyBarSkip + previewSkip) {
				if (likeService) {
					p.setPen(st::white->p);
				} else {
					p.setPen((selected ? (out() ? st::msgOutServiceSelColor : st::msgInServiceSelColor) : (out() ? st::msgOutServiceColor : st::msgInServiceColor))->p);
				}
				replyToName.drawElided(p, x + st::msgReplyBarSkip + previewSkip, y + st::msgReplyPadding.top(), w - st::msgReplyBarSkip - previewSkip);

				HistoryMessage *replyToAsMsg = replyToMsg->toHistoryMessage();
				if (likeService) {
				} else if ((replyToAsMsg && replyToAsMsg->justMedia()) || replyToMsg->serviceMsg()) {
					style::color date(selected ? (out() ? st::msgOutSelectDateColor : st::msgInSelectDateColor) : (out() ? st::msgOutDateColor : st::msgInDateColor));
					p.setPen(date->p);
				} else {
					p.setPen(st::msgColor->p);
				}
				replyToText.drawElided(p, x + st::msgReplyBarSkip + previewSkip, y + st::msgReplyPadding.top() + st::msgServiceNameFont->height, w - st::msgReplyBarSkip - previewSkip);
			}
		} else {
			p.setFont(st::msgDateFont->f);
			style::color date(selected ? (out() ? st::msgOutSelectDateColor : st::msgInSelectDateColor) : (out() ? st::msgOutDateColor : st::msgInDateColor));
			if (likeService) {
				p.setPen(st::white->p);
			} else {
				p.setPen(date->p);
			}
			p.drawText(x + st::msgReplyBarSkip, y + st::msgReplyPadding.top() + (st::msgReplyBarSize.height() - st::msgDateFont->height) / 2 + st::msgDateFont->ascent, st::msgDateFont->m.elidedText(lang(replyToMsgId ? lng_profile_loading : lng_deleted_message), Qt::ElideRight, w - st::msgReplyBarSkip));
		}
	}
}

void HistoryReply::drawMessageText(QPainter &p, const QRect &trect, uint32 selection) const {
	int32 h = st::msgReplyPadding.top() + st::msgReplyBarSize.height() + st::msgReplyPadding.bottom();

	drawReplyTo(p, trect.x(), trect.y(), trect.width(), (selection == FullItemSel));

	QRect realtrect(trect);
	realtrect.setY(trect.y() + h);
	HistoryMessage::drawMessageText(p, realtrect, selection);
}

int32 HistoryReply::resize(int32 width, bool dontRecountText, const HistoryItem *parent) {
	HistoryMessage::resize(width, dontRecountText, parent);
	if (!justMedia() && (_media || !dontRecountText)) {
		_height += st::msgReplyPadding.top() + st::msgReplyBarSize.height() + st::msgReplyPadding.bottom();
	}
	return _height;
}

bool HistoryReply::hasPoint(int32 x, int32 y) const {
	if (!justMedia()) {
		int32 left = out() ? st::msgMargin.right() : st::msgMargin.left(), width = _history->width - st::msgMargin.left() - st::msgMargin.right();
		if (width > st::msgMaxWidth) {
			if (out()) left += width - st::msgMaxWidth;
			width = st::msgMaxWidth;
		}

		if (!out() && _history->peer->chat) { // from user left photo
//			width -= st::msgPhotoSkip;
			left += st::msgPhotoSkip;
		}
		if (width < 1) return false;

		if (width >= _maxw) {
			if (out()) left += width - _maxw;
			width = _maxw;
		}
		QRect r(left, st::msgMargin.top(), width, _height - st::msgMargin.top() - st::msgMargin.bottom());
		return r.contains(x, y);
	}
	return HistoryMessage::hasPoint(x, y);
}

void HistoryReply::getState(TextLinkPtr &lnk, HistoryCursorState &state, int32 x, int32 y) const {
	lnk = TextLinkPtr();
	state = HistoryDefaultCursorState;

	if (!justMedia()) {
		int32 left = out() ? st::msgMargin.right() : st::msgMargin.left(), width = _history->width - st::msgMargin.left() - st::msgMargin.right();
		if (width > st::msgMaxWidth) {
			if (out()) left += width - st::msgMaxWidth;
			width = st::msgMaxWidth;
		}

		if (!out() && _history->peer->chat) { // from user left photo
			if (x >= left && x < left + st::msgPhotoSize) {
				return HistoryMessage::getState(lnk, state, x, y);
			}
//			width -= st::msgPhotoSkip;
			left += st::msgPhotoSkip;
		}
		if (width < 1) return;

		if (width >= _maxw) {
			if (out()) left += width - _maxw;
			width = _maxw;
		}
		QRect r(left, st::msgMargin.top(), width, _height - st::msgMargin.top() - st::msgMargin.bottom());
		if (!out() && _history->peer->chat) {
			style::font nameFont(st::msgNameFont);
			if (y >= r.top() + st::msgPadding.top() && y < r.top() + st::msgPadding.top() + nameFont->height) {
				return HistoryMessage::getState(lnk, state, x, y);
			}
			r.setTop(r.top() + nameFont->height);
		}
		QRect trect(r.marginsAdded(-st::msgPadding));

		int32 h = st::msgReplyPadding.top() + st::msgReplyBarSize.height() + st::msgReplyPadding.bottom();
		if (y >= trect.top() && y < trect.top() + h) {
			if (replyToMsg && y >= trect.top() + st::msgReplyPadding.top() && y < trect.top() + st::msgReplyPadding.top() + st::msgReplyBarSize.height() && x >= trect.left() && x < trect.right()) {
				lnk = replyToLnk;
			}
			return;
		}
		y -= h;
	}
	return HistoryMessage::getState(lnk, state, x, y);
}

void HistoryReply::getStateFromMessageText(TextLinkPtr &lnk, HistoryCursorState &state, int32 x, int32 y, const QRect &r) const {
	int32 h = st::msgReplyPadding.top() + st::msgReplyBarSize.height() + st::msgReplyPadding.bottom();

	QRect realr(r);
	realr.setHeight(r.height() - h);
	HistoryMessage::getStateFromMessageText(lnk, state, x, y, realr);
}

void HistoryReply::getSymbol(uint16 &symbol, bool &after, bool &upon, int32 x, int32 y) const {
	symbol = 0;
	after = false;
	upon = false;

	if (!justMedia()) {
		int32 left = out() ? st::msgMargin.right() : st::msgMargin.left(), width = _history->width - st::msgMargin.left() - st::msgMargin.right();
		if (width > st::msgMaxWidth) {
			if (out()) left += width - st::msgMaxWidth;
			width = st::msgMaxWidth;
		}

		if (!out() && _history->peer->chat) { // from user left photo
//			width -= st::msgPhotoSkip;
			left += st::msgPhotoSkip;
		}
		if (width < 1) return;

		if (width >= _maxw) {
			if (out()) left += width - _maxw;
			width = _maxw;
		}
		QRect r(left, st::msgMargin.top(), width, _height - st::msgMargin.top() - st::msgMargin.bottom());
		if (!out() && _history->peer->chat) {
			style::font nameFont(st::msgNameFont);
			if (y >= r.top() + st::msgPadding.top() && y < r.top() + st::msgPadding.top() + nameFont->height) {
				return HistoryMessage::getSymbol(symbol, after, upon, x, y);
			}
			r.setTop(r.top() + nameFont->height);
		}
		QRect trect(r.marginsAdded(-st::msgPadding));

		int32 h = st::msgReplyPadding.top() + st::msgReplyBarSize.height() + st::msgReplyPadding.bottom();
		y -= h;
	}
	return HistoryMessage::getSymbol(symbol, after, upon, x, y);
}

HistoryReply::~HistoryReply() {
	if (replyToMsg) {
		App::historyUnregReply(this, replyToMsg);
	} else if (replyToMsgId) {
		App::api()->itemRemoved(this);
	}
}

void HistoryServiceMsg::setMessageByAction(const MTPmessageAction &action) {
	TextLinkPtr second;
	LangString text = lang(lng_message_empty);
	QString from = textcmdLink(1, _from->name);

	switch (action.type()) {
	case mtpc_messageActionChatAddUser: {
		const MTPDmessageActionChatAddUser &d(action.c_messageActionChatAddUser());
		if (App::peerFromUser(d.vuser_id) == _from->id) {
			text = lng_action_user_joined(lt_from, from);
		} else {
			UserData *u = App::user(App::peerFromUser(d.vuser_id));
			second = TextLinkPtr(new PeerLink(u));
			text = lng_action_add_user(lt_from, from, lt_user, textcmdLink(2, u->name));
		}
	} break;

	case mtpc_messageActionChatJoinedByLink: {
		const MTPDmessageActionChatJoinedByLink &d(action.c_messageActionChatJoinedByLink());
		if (true || App::peerFromUser(d.vinviter_id) == _from->id) {
			text = lng_action_user_joined_by_link(lt_from, from);
		//} else {
			//UserData *u = App::user(App::peerFromUser(d.vinviter_id));
			//second = TextLinkPtr(new PeerLink(u));
			//text = lng_action_user_joined_by_link_from(lt_from, from, lt_inviter, textcmdLink(2, u->name));
		}
	} break;

	case mtpc_messageActionChatCreate: {
		const MTPDmessageActionChatCreate &d(action.c_messageActionChatCreate());
		text = lng_action_created_chat(lt_from, from, lt_title, textClean(qs(d.vtitle)));
	} break;

	case mtpc_messageActionChatDeletePhoto: {
		text = lng_action_removed_photo(lt_from, from);
	} break;

	case mtpc_messageActionChatDeleteUser: {
		const MTPDmessageActionChatDeleteUser &d(action.c_messageActionChatDeleteUser());
		if (App::peerFromUser(d.vuser_id) == _from->id) {
			text = lng_action_user_left(lt_from, from);
		} else {
			UserData *u = App::user(App::peerFromUser(d.vuser_id));
			second = TextLinkPtr(new PeerLink(u));
			text = lng_action_kick_user(lt_from, from, lt_user, textcmdLink(2, u->name));
		}
	} break;

	case mtpc_messageActionChatEditPhoto: {
		const MTPDmessageActionChatEditPhoto &d(action.c_messageActionChatEditPhoto());
		if (d.vphoto.type() == mtpc_photo) {
			_media = new HistoryPhoto(history()->peer, d.vphoto.c_photo(), st::msgServicePhotoWidth);
		}
		text = lng_action_changed_photo(lt_from, from);
	} break;

	case mtpc_messageActionChatEditTitle: {
		const MTPDmessageActionChatEditTitle &d(action.c_messageActionChatEditTitle());
		text = lng_action_changed_title(lt_from, from, lt_title, textClean(qs(d.vtitle)));
	} break;

	default: from = QString(); break;
	}

	_text.setText(st::msgServiceFont, text, _historySrvOptions);
	if (!from.isEmpty()) {
		_text.setLink(1, TextLinkPtr(new PeerLink(_from)));
	}
	if (second) {
		_text.setLink(2, second);
	}
	return ;
}

HistoryServiceMsg::HistoryServiceMsg(History *history, HistoryBlock *block, const MTPDmessageService &msg) :
	HistoryItem(history, block, msg.vid.v, msg.vflags.v, ::date(msg.vdate), msg.vfrom_id.v)
, _text(st::msgMinWidth)
, _media(0)
{
	setMessageByAction(msg.vaction);
}

HistoryServiceMsg::HistoryServiceMsg(History *history, HistoryBlock *block, MsgId msgId, QDateTime date, const QString &msg, int32 flags, HistoryMedia *media, int32 from) :
	HistoryItem(history, block, msgId, flags, date, from)
, _text(st::msgServiceFont, msg, _historySrvOptions, st::dlgMinWidth)
, _media(media)
{
}

void HistoryServiceMsg::initDimensions(const HistoryItem *parent) {
	_maxw = _text.maxWidth() + st::msgServicePadding.left() + st::msgServicePadding.right();
	_minh = _text.minHeight();
	if (_media) _media->initDimensions();
}

QString HistoryServiceMsg::selectedText(uint32 selection) const {
	uint16 selectedFrom = (selection == FullItemSel) ? 0 : (selection >> 16) & 0xFFFF;
	uint16 selectedTo = (selection == FullItemSel) ? 0xFFFF : (selection & 0xFFFF);
	return _text.original(selectedFrom, selectedTo);
}

QString HistoryServiceMsg::inDialogsText() const {
	return _text.original(0, 0xFFFF, false);
}

QString HistoryServiceMsg::inReplyText() const {
	QString result = HistoryServiceMsg::inDialogsText();
	return result.trimmed().startsWith(from()->name) ? result.trimmed().mid(from()->name.size()).trimmed() : result;
}

void HistoryServiceMsg::draw(QPainter &p, uint32 selection) const {
	uint64 ms = App::main() ? App::main()->animActiveTime(id) : 0;
	if (ms) {
		if (ms > st::activeFadeInDuration + st::activeFadeOutDuration) {
			App::main()->stopAnimActive();
		} else {
			textstyleSet(&st::inTextStyle);
			float64 dt = (ms > st::activeFadeInDuration) ? (1 - (ms - st::activeFadeInDuration) / float64(st::activeFadeOutDuration)) : (ms / float64(st::activeFadeInDuration));
			float64 o = p.opacity();
			p.setOpacity(o * dt);
			p.fillRect(0, 0, _history->width, _height, textstyleCurrent()->selectOverlay->b);
			p.setOpacity(o);
		}
	}

	textstyleSet(&st::serviceTextStyle);

	int32 left = st::msgServiceMargin.left(), width = _history->width - st::msgServiceMargin.left() - st::msgServiceMargin.left(), height = _height - st::msgServiceMargin.top() - st::msgServiceMargin.bottom(); // two small margins
	if (width < 1) return;

	if (_media) {
		height -= st::msgServiceMargin.top() + _media->height();
		p.save();
		p.translate(st::msgServiceMargin.left() + (width - _media->maxWidth()) / 2, st::msgServiceMargin.top() + height + st::msgServiceMargin.top());
		_media->draw(p, this, selection == FullItemSel);
		p.restore();
	}

	QRect trect(QRect(left, st::msgServiceMargin.top(), width, height).marginsAdded(-st::msgServicePadding));

	if (width > _maxw) {
		left += (width - _maxw) / 2;
		width = _maxw;
	}
	App::roundRect(p, left, st::msgServiceMargin.top(), width, height, App::msgServiceBg(), (selection == FullItemSel) ? ServiceSelectedCorners : ServiceCorners);

	p.setBrush(Qt::NoBrush);
	p.setPen(st::msgServiceColor->p);
	p.setFont(st::msgServiceFont->f);
	uint16 selectedFrom = (selection == FullItemSel) ? 0 : (selection >> 16) & 0xFFFF;
	uint16 selectedTo = (selection == FullItemSel) ? 0 : selection & 0xFFFF;
	_text.draw(p, trect.x(), trect.y(), trect.width(), Qt::AlignCenter, 0, -1, selectedFrom, selectedTo);
	textstyleRestore();
}

int32 HistoryServiceMsg::resize(int32 width, bool dontRecountText, const HistoryItem *parent) {
	if (dontRecountText) return _height;

	width -= st::msgServiceMargin.left() + st::msgServiceMargin.left(); // two small margins
	if (width < st::msgServicePadding.left() + st::msgServicePadding.right() + 1) width = st::msgServicePadding.left() + st::msgServicePadding.right() + 1;

	int32 nwidth = qMax(width - st::msgPadding.left() - st::msgPadding.right(), 0);
	if (nwidth != _textWidth) {
		_textWidth = nwidth;
		_textHeight = _text.countHeight(nwidth);
	}
	if (width >= _maxw) {
		_height = _minh;
	} else {
		_height = _textHeight;
	}
	_height += st::msgServicePadding.top() + st::msgServicePadding.bottom() + st::msgServiceMargin.top() + st::msgServiceMargin.bottom();
	if (_media) {
		_height += st::msgServiceMargin.top() + _media->resize(_media->currentWidth());
	}
	return _height;
}

bool HistoryServiceMsg::hasPoint(int32 x, int32 y) const {
	int32 left = st::msgServiceMargin.left(), width = _history->width - st::msgServiceMargin.left() - st::msgServiceMargin.left(), height = _height - st::msgServiceMargin.top() - st::msgServiceMargin.bottom(); // two small margins
	if (width < 1) return false;

	if (_media) {
		height -= st::msgServiceMargin.top() + _media->height();
	}
	return QRect(left, st::msgServiceMargin.top(), width, height).contains(x, y);
}

void HistoryServiceMsg::getState(TextLinkPtr &lnk, HistoryCursorState &state, int32 x, int32 y) const {
	lnk = TextLinkPtr();
	state = HistoryDefaultCursorState;

	int32 left = st::msgServiceMargin.left(), width = _history->width - st::msgServiceMargin.left() - st::msgServiceMargin.left(), height = _height - st::msgServiceMargin.top() - st::msgServiceMargin.bottom(); // two small margins
	if (width < 1) return;

	if (_media) {
		height -= st::msgServiceMargin.top() + _media->height();
	}
	QRect trect(QRect(left, st::msgServiceMargin.top(), width, height).marginsAdded(-st::msgServicePadding));
	if (trect.contains(x, y)) {
		bool inText = false;
		_text.getState(lnk, inText, x - trect.x(), y - trect.y(), trect.width(), Qt::AlignCenter);
		state = inText ? HistoryInTextCursorState : HistoryDefaultCursorState;
	} else if (_media) {
		_media->getState(lnk, state, x - st::msgServiceMargin.left() - (width - _media->maxWidth()) / 2, y - st::msgServiceMargin.top() - height - st::msgServiceMargin.top(), this);
	}
}

void HistoryServiceMsg::getSymbol(uint16 &symbol, bool &after, bool &upon, int32 x, int32 y) const {
	symbol = 0;
	after = false;
	upon = false;

	int32 left = st::msgServiceMargin.left(), width = _history->width - st::msgServiceMargin.left() - st::msgServiceMargin.left(), height = _height - st::msgServiceMargin.top() - st::msgServiceMargin.bottom(); // two small margins
	if (width < 1) return;

	if (_media) {
		height -= st::msgServiceMargin.top() + _media->height();
	}
	QRect trect(QRect(left, st::msgServiceMargin.top(), width, height).marginsAdded(-st::msgServicePadding));
	return _text.getSymbol(symbol, after, upon, x - trect.x(), y - trect.y(), trect.width(), Qt::AlignCenter);
}

void HistoryServiceMsg::drawInDialog(QPainter &p, const QRect &r, bool act, const HistoryItem *&cacheFor, Text &cache) const {
	if (cacheFor != this) {
		cacheFor = this;
		cache.setText(st::dlgHistFont, inDialogsText(), _textDlgOptions);
	}
	QRect tr(r);
	p.setPen((act ? st::dlgActiveColor : st::dlgSystemColor)->p);
	cache.drawElided(p, tr.left(), tr.top(), tr.width(), tr.height() / st::dlgHistFont->height);
}

QString HistoryServiceMsg::notificationText() const {
    QString msg = _text.original(0, 0xFFFF);
    if (msg.size() > 0xFF) msg = msg.mid(0, 0xFF) + qsl("..");
    return msg;
}

HistoryMedia *HistoryServiceMsg::getMedia(bool inOverview) const {
	return inOverview ? 0 : _media;
}

HistoryServiceMsg::~HistoryServiceMsg() {
	delete _media;
}

HistoryDateMsg::HistoryDateMsg(History *history, HistoryBlock *block, const QDate &date) : HistoryServiceMsg(history, block, clientMsgId(), QDateTime(date), langDayOfMonth(date)) {
}

HistoryItem *createDayServiceMsg(History *history, HistoryBlock *block, QDateTime date) {
	return regItem(new HistoryDateMsg(history, block, date.date()));
}

HistoryUnreadBar::HistoryUnreadBar(History *history, HistoryBlock *block, int32 count, const QDateTime &date) : HistoryItem(history, block, clientMsgId(), 0, date, 0), freezed(false) {
	setCount(count);
	initDimensions();
}

void HistoryUnreadBar::initDimensions(const HistoryItem *parent) {
	_maxw = st::msgPadding.left() + st::msgPadding.right() + 1;
	_minh = st::unreadBarHeight;
}

void HistoryUnreadBar::setCount(int32 count) {
	if (!count) freezed = true;
	if (freezed) return;
	text = lng_unread_bar(lt_count, count);
}

void HistoryUnreadBar::draw(QPainter &p, uint32 selection) const {
	p.fillRect(0, st::lineWidth, _history->width, st::unreadBarHeight - 2 * st::lineWidth, st::unreadBarBG->b);
	p.fillRect(0, st::unreadBarHeight - st::lineWidth, _history->width, st::lineWidth, st::unreadBarBorder->b);
	p.setFont(st::unreadBarFont->f);
	p.setPen(st::unreadBarColor->p);
	p.drawText(QRect(0, 0, _history->width, st::unreadBarHeight - st::lineWidth), text, style::al_center);
}

int32 HistoryUnreadBar::resize(int32 width, bool dontRecountText, const HistoryItem *parent) {
	_height = st::unreadBarHeight;
	return _height;
}

void HistoryUnreadBar::drawInDialog(QPainter &p, const QRect &r, bool act, const HistoryItem *&cacheFor, Text &cache) const {
}

QString HistoryUnreadBar::notificationText() const {
    return QString();
}

