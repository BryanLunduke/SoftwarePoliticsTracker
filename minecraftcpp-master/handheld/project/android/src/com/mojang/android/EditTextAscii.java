package com.mojang.android;

import android.content.Context;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.AttributeSet;
import android.widget.EditText;

public class EditTextAscii	extends EditText
							implements TextWatcher {

	public EditTextAscii(Context context) {
		super(context);
		_init();
	}

	public EditTextAscii(Context context, AttributeSet attrs) {
		super(context, attrs);
		_init();
	}
	
	public EditTextAscii(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
		_init();
	}
	
	private void _init() {
		this.addTextChangedListener(this);
	}

	//
	// TextWatcher
	//
	@Override
	public void onTextChanged(CharSequence s, int start, int before, int count) {}
	//@Override
	public void beforeTextChanged(CharSequence s, int start, int count, int after) { }
	//@Override
	public void afterTextChanged(Editable e) {
		String s = e.toString();
		String sanitized = sanitize(s);

		if (!s.equals(sanitized)) {
			e.replace(0, e.length(), sanitized);
		}
	}

	static public String sanitize(String s) {
		StringBuilder sb = new StringBuilder(s.length());

		for (int i = 0; i < s.length(); ++i) {
			char ch = s.charAt(i);
			if (ch < 128)
				sb.append(ch);
		}
		return sb.toString();
	}
}
