package com.mojang.android.preferences;

import android.content.Context;
import android.content.res.Resources;
import android.preference.DialogPreference;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.TextView;

public class SliderPreference extends DialogPreference implements SeekBar.OnSeekBarChangeListener {

	private static final String androidns = "http://schemas.android.com/apk/res/android";

	private Context _context;
	
	//private TextView _minValueTextView;
	private TextView _valueTextView;
	//private TextView _maxValueTextView;
	private SeekBar _seekBar;

	private String _valueSuffix;
	
	private int _defaultValue;	// Stores the default preference value when none has been set
	private int _maxValue;		// Stores the upper preference value bound
	private int _value;			// Stores the value of the preference
	private int _minValue;		// Stores the minimum preference value bound

	public SliderPreference(Context context, AttributeSet attrs) {
		super(context, attrs);

		_context = context;
		
		_valueSuffix = getResourceValueFromAttribute(attrs, androidns, "text", "");
		
		_defaultValue = getResourceValueFromAttribute(attrs, androidns, "defaultValue", 0);
		_maxValue = getResourceValueFromAttribute(attrs, androidns, "max", 100);
		_minValue = getResourceValueFromAttribute(attrs, null, "min", 0);
				
		// Set the default value of the preference to the default value found in attribute
		setDefaultValue((Integer) _defaultValue);
	}
	//public SliderPreference(Context context, AttributeSet attrs, int)

	@Override 
	protected View onCreateDialogView() {
	    LinearLayout.LayoutParams params;
	    LinearLayout layout = new LinearLayout(getContext());
	    layout.setOrientation(LinearLayout.VERTICAL);
	    layout.setPadding(6,6,6,6);

//	    mSplashText = new TextView(_context);
//	    if (mDialogMessage != null)
//	      mSplashText.setText(mDialogMessage);
//	    layout.addView(mSplashText);

	    _valueTextView = new TextView(_context);
	    _valueTextView.setGravity(Gravity.CENTER_HORIZONTAL);
	    _valueTextView.setTextSize(32);
	    params = new LinearLayout.LayoutParams(
	        LinearLayout.LayoutParams.FILL_PARENT, 
	        LinearLayout.LayoutParams.WRAP_CONTENT);
	    layout.addView(_valueTextView, params);

	    _seekBar = new SeekBar(_context);
	    _seekBar.setOnSeekBarChangeListener(this);
	    layout.addView(_seekBar, new LinearLayout.LayoutParams(LinearLayout.LayoutParams.FILL_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT));

	    if (shouldPersist())
	      _value = getPersistedInt(_defaultValue);

	    _seekBar.setMax(_maxValue);
	    _seekBar.setProgress(_value);
	    return layout;
	  }	
	
	@Override
	protected void onSetInitialValue(boolean restore, Object defaultValue) {
		super.onSetInitialValue(restore, defaultValue);
		if (restore)
			_value = shouldPersist() ? getPersistedInt(_defaultValue) : 0;
		else
			_value = (Integer) defaultValue;
	}
	
	
/*
	@Override
	protected void onBindView(View view) {
		super.onBindView(view);
		// Bind _seekBar to the layout's internal view
		_seekBar = (SeekBar) view.findViewById(R.id.slider_preference_seekbar);

		// Setup it's listeners and parameters
		_seekBar.setMax(translateValueToSeekBar(_maxValue));
		_seekBar.setProgress(translateValueToSeekBar(_value));
		_seekBar.setOnSeekBarChangeListener(this);
		
		// Bind mTextView to the layout's internal view
		_valueTextView = (TextView) view.findViewById(R.id.slider_preference_value);
		
		// Setup it's parameters
		_valueTextView.setText(getValueText(_value));
		
		// Setup min and max value text views
		_minValueTextView = (TextView) view.findViewById(R.id.slider_preference_min_value);
		_minValueTextView.setText(getValueText(_minValue));
		_maxValueTextView = (TextView) view.findViewById(R.id.slider_preference_max_value);
		_maxValueTextView.setText(getValueText(_maxValue));
	}
*/
	
	//@Override
	public void onProgressChanged(SeekBar seekBar, int value, boolean fromUser) {
		// Change mTextView and _value to the current seekbar value
		_value = translateValueFro_seekBar(value);
		_valueTextView.setText(getValueText(_value));
		if (shouldPersist())
			persistInt(translateValueFro_seekBar(value));
		callChangeListener(new Integer(_value));
	}

	//@Override
	public void onStartTrackingTouch(SeekBar seekBar) {
		// Not used
	}

	//@Override
	public void onStopTrackingTouch(SeekBar seekBar) {
		// Not used
	}
	
	private String getValueText(int value) {
		String t = String.valueOf(value);
		return t.concat(_valueSuffix);
	}
	
	/**
	 * Retrieves a value from the resources of this slider preference's context
	 * based on an attribute pointing to that resource.
	 * 
	 * @param attrs
	 * The attribute set to search
	 * 
	 * @param namespace
	 * The namespace of the attribute
	 * 
	 * @param name
	 * The name of the attribute
	 * 
	 * @param defaultValue
	 * The default value returned if no resource is found
	 * 
	 * @return
	 * The resource value
	 */
	private int getResourceValueFromAttribute(AttributeSet attrs, String namespace, String name, int defaultValue) {
		Resources res = getContext().getResources();
		int valueID = attrs.getAttributeResourceValue(namespace, name, 0);
		if (valueID != 0) {
			return res.getInteger(valueID);
		}
		else {
			return attrs.getAttributeIntValue(namespace, name, defaultValue);
		}
	}
	
	/**
	 * Retrieves a value from the resources of this slider preference's context
	 * based on an attribute pointing to that resource.
	 * 
	 * @param attrs
	 * The attribute set to search
	 * 
	 * @param namespace
	 * The namespace of the attribute
	 * 
	 * @param name
	 * The name of the attribute
	 * 
	 * @param defaultValue
	 * The default value returned if no resource is found
	 * 
	 * @return
	 * The resource value
	 */
	private String getResourceValueFromAttribute(AttributeSet attrs, String namespace, String name, String defaultValue) {
		Resources res = getContext().getResources();
		int valueID = attrs.getAttributeResourceValue(namespace, name, 0);
		if (valueID != 0) {
			return res.getString(valueID);
		}
		else {
			String value = attrs.getAttributeValue(namespace, name);
			if (value != null) {
				return value; 
			}
			else {
				return defaultValue;
			}
		}
	}

	/**
	 * Translates a value from this slider preference's seekbar by
	 * adjusting it for a set perceived minimum value.
	 * 
	 * @param value
	 * The value to be translated from the seekbar
	 * 
	 * @return
	 * A value equal to the value passed plus the currently set minimum value.
	 */
	private int translateValueFro_seekBar(int value) {
		return value + _minValue;
	}
	
	/**
	 * Translates a value for when setting this slider preference's seekbar by
	 * adjusting it for a set perceived minimum value.
	 *
	 * @param value
	 * The value to be translated for use
	 *
	 * @return
	 * A value equal to the value passed minus the currently set minimum value.
	 */
	private int translateValueToSeekBar(int value) {
		return value - _minValue;
	}
}
