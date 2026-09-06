package com.mojang.minecraftpe;

import com.mojang.android.StringValue;
import com.mojang.minecraftpe.R;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.TextView;
import android.widget.ToggleButton;

public class GameModeButton extends ToggleButton implements OnClickListener, StringValue {
	public GameModeButton(Context context, AttributeSet attrs) {
		super(context, attrs);
		_init();
	}
	
	//@Override
	public void onClick(View v) {
		_update();
	}
	@Override
	protected void onFinishInflate() {
		super.onFinishInflate();
		_update();
	}
	@Override
	protected void onAttachedToWindow() {
		if (!_attached) {
			_update();
			_attached = true;
		}
	}
	private boolean _attached = false;
	
	private void _init() {
		setOnClickListener(this);
	}
	private void _update() {
		_setGameType(isChecked()?Survival:Creative);
	}
	private void _setGameType(int i) {
		_type = _clamp(i);

		int id =  R.string.gamemode_creative_summary;
		if (_type == Survival)
			id = R.string.gamemode_survival_summary;
		String desc = getContext().getString(id);

		View v = getRootView().findViewById(R.id.labelGameModeDesc);
		System.out.println("Mode: " + _type + ", view? " + (v!=null));
		if (desc != null && v != null && v instanceof TextView) {
			((TextView)v).setText(desc);
		}
	}

	static private int _clamp(int i) {
		if (i > Survival) return Survival;
		if (i < Creative) return Creative;
		return i;
	}
	
	public String getStringValue() {
		return getStringForType(_type);
	}
	static public String getStringForType(int i) {
		return new String[] {
	        "creative",
	        "survival"
	    } [_clamp(i)];
	}
	
	private int _type = 0;
	static final int Creative = 0;
	static final int Survival = 1;
}
