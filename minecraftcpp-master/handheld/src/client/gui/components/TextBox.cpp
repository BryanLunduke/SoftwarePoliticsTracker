#include "TextBox.h"
#include "../../Minecraft.h"
#include "../../../AppPlatform.h"
#include "../../../platform/log.h"
#include <algorithm>

TextBox::TextBox( int id, const std::string& msg )
 : id(id), w(0), h(0), x(0), y(0), text(msg), focused(false), caretPos((int)msg.length()) {

}

TextBox::TextBox( int id, int x, int y, const std::string& msg ) 
 : id(id), w(0), h(0), x(x), y(y), text(msg), focused(false), caretPos((int)msg.length()) {

}

TextBox::TextBox( int id, int x, int y, int w, int h, const std::string& msg )
 : id(id), w(w), h(h), x(x), y(y), text(msg), focused(false), caretPos((int)msg.length()) {

}

void TextBox::setFocus(Minecraft* minecraft) {
	if(!focused) {
		if (minecraft && minecraft->platform()) minecraft->platform()->showKeyboard();
		focused = true;
		if (caretPos <0) caretPos =0;
		if (caretPos > (int)text.length()) caretPos = (int)text.length();
	}
}

bool TextBox::loseFocus(Minecraft* minecraft) {
	if(focused) {
		if (minecraft && minecraft->platform()) minecraft->platform()->hideKeyboard();
		focused = false;
		return true;
	}
	return false;
}

void TextBox::setCaretPos(int p) {
	if (p <0) p =0;
	if (p > (int)text.length()) p = (int)text.length();
	caretPos = p;
}

void TextBox::insertChar(char c) {
	LOGW("TextBox::insertChar id=%d before='%s' caret=%d inserting='%c'(%d)", id, text.c_str(), caretPos, (c >=32 && c <127) ? c : '?', (int)(unsigned char)c);
	text.insert(text.begin() + caretPos, c);
	++caretPos;
	LOGW("TextBox::insertChar id=%d after='%s' caret=%d", id, text.c_str(), caretPos);
}

void TextBox::backspaceAtCaret() {
	LOGW("TextBox::backspaceAtCaret id=%d before='%s' caret=%d", id, text.c_str(), caretPos);
	if (caretPos >0) {
		text.erase(text.begin() + caretPos -1);
		--caretPos;
		LOGW("TextBox::backspaceAtCaret id=%d after='%s' caret=%d", id, text.c_str(), caretPos);
	} else {
		LOGW("TextBox::backspaceAtCaret id=%d no-op (caret==0)", id);
	}
}

void TextBox::deleteAtCaret() {
	LOGW("TextBox::deleteAtCaret id=%d before='%s' caret=%d", id, text.c_str(), caretPos);
	if (caretPos < (int)text.length()) {
		text.erase(text.begin() + caretPos);
		LOGW("TextBox::deleteAtCaret id=%d after='%s' caret=%d", id, text.c_str(), caretPos);
	} else {
		LOGW("TextBox::deleteAtCaret id=%d no-op (caret at end)", id);
	}
}

void TextBox::moveCaretLeft() {
	if (caretPos >0) --caretPos;
}

void TextBox::moveCaretRight() {
	if (caretPos < (int)text.length()) ++caretPos;
}

void TextBox::moveCaretHome() { caretPos =0; }
void TextBox::moveCaretEnd() { caretPos = (int)text.length(); }

void TextBox::setCaretFromPixel(int px, Font* font) {
	// Use font metrics to determine nearest character position
	if (!font) {
		// fallback to approximate
		int relative = px - (x +4);
		int pos = (relative >0) ? (relative /6) :0;
		if (pos <0) pos =0;
		if (pos > (int)text.length()) pos = (int)text.length();
		caretPos = pos;
		return;
	}
	int rel = px - (x +4);
	if (rel <=0) { caretPos =0; return; }
	int best =0;
	int bestDiff = INT_MAX;
	for (int i =0; i <= (int)text.length(); ++i) {
		int w = font->width(text.substr(0, i));
		int diff = abs(w - rel);
		if (diff < bestDiff) { best = i; bestDiff = diff; }
		if (w > rel && i >0 && diff > bestDiff) break;
	}
	if (best <0) best =0;
	if (best > (int)text.length()) best = (int)text.length();
	caretPos = best;
}

void TextBox::render( Minecraft* minecraft, int xm, int ym ) {
	if (w ==0) {
		// default width
		// try to not overflow the screen
		w =120;
	}
	if (h ==0) h =20;

	// Background box
	fill(x, y, x + w, y + h,0x80000000);
	// Border - use different colors when focused
	if (focused) {
		fill(x, y, x + w, y +1,0xffa0c0ff);
		fill(x, y + h -1, x + w, y + h,0xff4060a0);
		fill(x, y, x +1, y + h,0xffa0c0ff);
		fill(x + w -1, y, x + w, y + h,0xff4060a0);
	} else {
		fill(x, y, x + w, y +1,0xff707070);
		fill(x, y + h -1, x + w, y + h,0xff101010);
		fill(x, y, x +1, y + h,0xff707070);
		fill(x + w -1, y, x + w, y + h,0xff101010);
	}

	int color =0xe0e0e0;
	if (!focused && (text.empty())) color =0xffa0a0a0;

	// Clip text if too long - simple truncate
	std::string out = text;
	int maxChars =1;
	if (minecraft && minecraft->font) {
		int avgCharWidth = minecraft->font->width(std::string("W"));
		if (avgCharWidth <=0) avgCharWidth =6;
		int avg = (w -8) / avgCharWidth;
		if (avg > maxChars) maxChars = avg;
	} else {
		int calc = (w -8) /6; // approx6 px per char
		if (calc > maxChars) maxChars = calc;
	}
	if ((int)out.size() > maxChars) {
		out = out.substr(0, maxChars -3) + "...";
	}

	drawString(minecraft->font, out, x +4, y + (h -8) /2, color);

	// Draw caret if focused
	if (focused) {
		int caretPixel = x +4;
		if (minecraft && minecraft->font) {
			caretPixel += minecraft->font->width(text.substr(0, caretPos));
		} else {
			caretPixel += caretPos *6;
		}
		if (caretPixel > x + w -6) caretPixel = x + w -6;
		// draw vertical1px line as caret
		fill(caretPixel, y +3, caretPixel +1, y + h -4,0xFFFFFFFF);
	}
}
