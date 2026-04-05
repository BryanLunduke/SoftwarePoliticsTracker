package com.mojang.minecraftpe;

import java.util.ArrayList;

import com.mojang.android.EditTextAscii;
//import com.mojang.minecraftpe.R;

import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.EditTextPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceCategory;
import android.preference.PreferenceGroup;
import android.preference.PreferenceManager;
import android.preference.PreferenceScreen;

public class MainMenuOptionsActivity extends PreferenceActivity implements
 				SharedPreferences.OnSharedPreferenceChangeListener
{
	static public final String Multiplayer_Username = "mp_username";  
	static public final String Multiplayer_ServerVisible = "mp_server_visible_default";
	static public final String Graphics_Fancy = "gfx_fancygraphics";
	static public final String Graphics_LowQuality = "gfx_lowquality";
	static public final String Controls_InvertMouse = "ctrl_invertmouse";
	static public final String Controls_Sensitivity = "ctrl_sensitivity";
	static public final String Controls_UseTouchscreen = "ctrl_usetouchscreen";
	static public final String Controls_UseTouchJoypad = "ctrl_usetouchjoypad";
	static public final String Controls_FeedbackVibration = "feedback_vibration";
	static public final String Game_DifficultyLevel = "game_difficulty";
	static public final String Internal_Game_DifficultyPeaceful = "game_difficultypeaceful";
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
	    super.onCreate(savedInstanceState);  

	    Bundle extras = getIntent().getExtras();
	    addPreferencesFromResource(extras.getInt("preferenceId"));//R.xml.preferences);
	
	    //getPreferenceManager().setSharedPreferencesMode(MODE_PRIVATE);
	     
	    PreferenceManager.getDefaultSharedPreferences(this).registerOnSharedPreferenceChangeListener(this);
	    PreferenceScreen s = getPreferenceScreen();

	    if (PreferenceManager.getDefaultSharedPreferences(this).contains(Multiplayer_Username)) {
	    	previousName = PreferenceManager.getDefaultSharedPreferences(this).getString(Multiplayer_Username, null);
	    }

	    _validator = new PreferenceValidator(this);
	    readBackAll(s);
	    _validator.commit();
	}

	private void readBackAll(PreferenceGroup g) {
		traversePreferences(g, new PreferenceTraverser() {
			void onPreference(Preference p) { readBack(p); _validator.validate(p); } 
		});
	}
	private void traversePreferences(PreferenceGroup g, PreferenceTraverser pt) {
	     int size = g.getPreferenceCount();
	     for (int i = 0; i < size; ++i) {
	    	 Preference p = g.getPreference(i);
	    	 if (p instanceof PreferenceGroup) {
	    		 PreferenceGroup pg = (PreferenceGroup)p;
	    		 pt.onPreferenceGroup(pg);
	    		 traversePreferences(pg, pt);
	    	 }
	    	 else
	    		 pt.onPreference(p);
	     }
	}
	private void readBack(Preference p) {
		if (p == null)
			return;

		//System.out.println("pref: " + p.toString());
			
		if (p instanceof EditTextPreference) {
		    EditTextPreference e = (EditTextPreference) p;
		    p.setSummary("'" + e.getText() + "'");
		}
	}

	//@Override
	public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
		Preference p = findPreference(key);
		_validator.validate(sharedPreferences, key);

		if (p instanceof EditTextPreference) {
			EditTextPreference e = (EditTextPreference) p;
			Editor editor = sharedPreferences.edit();

			String s = e.getText();
			String sanitized = EditTextAscii.sanitize(s).trim();

			if (key.equals(Multiplayer_Username) && sanitized == null || sanitized.length() == 0) {
				sanitized = previousName;
				if (sanitized == null || sanitized.equals("")) {
					sanitized = "Steve";
					previousName = sanitized;
				}
			}

			if (!s.equals(sanitized)) {
				editor.putString(key, sanitized);
				editor.commit();
				e.setText(sanitized);
			}
		}

		readBack(p);
	}
	 
	String previousName;
	PreferenceValidator _validator;
}

class PreferenceValidator {
	static private class Pref {
		Pref(PreferenceGroup g, Preference p) {
			this.g = g;
			this.p = p;
		}
		PreferenceGroup g;
		Preference p;
	}
	private PreferenceActivity _prefs;
	private ArrayList<Pref> _arrayList = new ArrayList<Pref>();
	
	public PreferenceValidator(PreferenceActivity prefs) {
		_prefs = prefs;
	}

	public void commit() {
		//System.err.println("ERR: " + _arrayList.size());
		for (int i = 0; i < _arrayList.size(); ++i) {
			PreferenceGroup g = _arrayList.get(i).g;
			Preference p = _arrayList.get(i).p;
			g.removePreference(p);
		}
	}

	public void validate(Preference p) {
		validate(PreferenceManager.getDefaultSharedPreferences(_prefs), p.getKey());
	}
	public void validate(SharedPreferences preferences, String key) {
		Preference p = findPreference(key);

		if (p instanceof CheckBoxPreference) {
			//CheckBoxPreference e = (CheckBoxPreference) p;
			if (key.equals(MainMenuOptionsActivity.Graphics_LowQuality)) {
				boolean isShort = preferences.getBoolean(MainMenuOptionsActivity.Graphics_LowQuality, false);
				CheckBoxPreference fancyPref = (CheckBoxPreference)findPreference(MainMenuOptionsActivity.Graphics_Fancy);
				if (fancyPref != null) {
					fancyPref.setEnabled(isShort == false);
					if (isShort)
						fancyPref.setChecked(false);
				}
			}
			if (key.equals(MainMenuOptionsActivity.Graphics_Fancy)) {
				CheckBoxPreference fancyPref = (CheckBoxPreference) p;
				//System.err.println("Is PowerVR? : " + MainActivity.isPowerVR());
				if (MainActivity.isPowerVR()) {
					fancyPref.setSummary("Experimental on this device!");
				}
			}
		
			if (p.getKey().equals(MainMenuOptionsActivity.Controls_UseTouchscreen)) {
				boolean hasOtherPrimaryControls = MainActivity.isXperiaPlay();
				if (!hasOtherPrimaryControls) {
					PreferenceCategory mCategory = (PreferenceCategory) findPreference("category_graphics");
					_arrayList.add(new Pref(mCategory, p));
				}
				p.setEnabled(hasOtherPrimaryControls);
				p.setDefaultValue( !hasOtherPrimaryControls );

				if (hasOtherPrimaryControls) {
					CheckBoxPreference pp = (CheckBoxPreference) p;
					CheckBoxPreference j = (CheckBoxPreference)findPreference(MainMenuOptionsActivity.Controls_UseTouchJoypad);
					j.setEnabled(pp.isChecked());
				}
			}
		}
	}

	private Preference findPreference(String key) {
		return _prefs.findPreference(key);
	}
}

abstract class PreferenceTraverser {
	 void onPreference(Preference p) {}
	 void onPreferenceGroup(PreferenceGroup p) {}
 }
 