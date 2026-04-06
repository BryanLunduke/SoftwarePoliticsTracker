package com.mojang.minecraftpe;

import java.io.BufferedInputStream;
import java.io.ByteArrayOutputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.Map;
import java.util.Vector;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import com.mojang.android.StringValue;
import com.mojang.android.licensing.LicenseCodes;

import android.app.ActivityManager;
import android.app.ActivityManager.MemoryInfo;
import android.app.AlertDialog;
import android.app.Activity;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnCancelListener;
import android.content.DialogInterface.OnClickListener;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.Bitmap.CompressFormat;
import android.graphics.BitmapFactory;
import android.media.AudioManager;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.os.Vibrator;
import android.preference.PreferenceManager;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Display;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.view.ViewGroup.LayoutParams;
import android.widget.EditText;
import android.widget.TextView;

import com.mojang.minecraftpe.R;
import com.mojang.minecraftpe.sound.SoundPlayer;

public class MainActivity extends Activity {
	/** Called when the activity is first created. */
    
    private GLView _glView;
    public float invScale = 1.0f;// / 1.5f;
    
    Vector<MotionEvent> _touchEvents = new Vector<MotionEvent>();
    Vector<KeyEvent> _keyEvents = new Vector<KeyEvent>();

    @Override
    public void onCreate(Bundle savedInstanceState) {
        setVolumeControlStream(AudioManager.STREAM_MUSIC);
        super.onCreate(savedInstanceState);
        nativeRegisterThis();
        
        nativeOnCreate();

        _glView = new GLView(getApplication(), this);
        //_glView.setEGLConfigChooser(8, 8, 8, 8, 16, 0);
        _glView.setEGLConfigChooser(true);
        //_glView
        
//        _glView.setEGLConfigChooser(
//        	new GLSurfaceView.EGLConfigChooser() {
//			
//			@Override
//			public EGLConfig chooseConfig(EGL10 egl, EGLDisplay display) {
//				// TODO Auto-generated method stub
//				
//		        // Specify a configuration for our opengl session 
//		         // and grab the first configuration that matches is 
//		         int[] configSpec = { 
//		                 EGL10.EGL_DEPTH_SIZE,   24, 
//		                 EGL10.EGL_NONE 
//		         }; 
//		         EGLConfig[] configs = new EGLConfig[1]; 
//		         int[] num_config = new int[1]; 
//		         egl.eglChooseConfig(display, configSpec, configs, 1, num_config); 
//		         EGLConfig config = configs[0];
//		         return config;
//				
//				//return null;
//			}
//		} );

        _glView.commit();
    	setContentView(_glView);
    	
    	_soundPlayer = new SoundPlayer(this, AudioManager.STREAM_MUSIC);
    }

    public boolean isTouchscreen() { return true; }
    static public boolean isXperiaPlay() { return false; }

    static private boolean _isPowerVr = false;
    public void setIsPowerVR(boolean status) { MainActivity._isPowerVr = status; }
    static public boolean isPowerVR() { return _isPowerVr; }
    
    public void vibrate(int milliSeconds) {
    	Vibrator v = (Vibrator)this.getSystemService(VIBRATOR_SERVICE);
    	v.vibrate(milliSeconds);
    }

    private void createAlertDialog(boolean hasOkButton, boolean hasCancelButton, boolean preventBackKey) {
    	AlertDialog.Builder builder = new AlertDialog.Builder(this);

    	builder.setTitle("");
    	if (preventBackKey)
    		builder.setCancelable(false);

    	builder.setOnCancelListener(new OnCancelListener() {
			//@Override
			public void onCancel(DialogInterface dialog) {
				onDialogCanceled();
			}
		});

    	if (hasOkButton)
    		builder.setPositiveButton("Ok", new OnClickListener() {
				public void onClick(DialogInterface dialog, int which) { onDialogCompleted(); }});

    	if (hasCancelButton)
	    	builder.setNegativeButton("Cancel", new OnClickListener() {
				public void onClick(DialogInterface dialog, int which) { onDialogCanceled(); }});

    	mDialog = builder.create();
    	mDialog.setOwnerActivity(this);
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
    	// TODO Auto-generated method stub
    	//System.out.println("Focus has changed. Has Focus? " + hasFocus);
    	super.onWindowFocusChanged(hasFocus);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
    	if (event.getRepeatCount() <= 0)
			_keyEvents.add(new KeyEvent(event));

    	boolean handled = (keyCode != KeyEvent.KEYCODE_VOLUME_DOWN
    					&& keyCode != KeyEvent.KEYCODE_VOLUME_UP);
    	return handled;
    }
    
    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
    	if (event.getRepeatCount() <= 0)
			_keyEvents.add(new KeyEvent(event));

    	boolean handled = (keyCode != KeyEvent.KEYCODE_VOLUME_DOWN
    					&& keyCode != KeyEvent.KEYCODE_VOLUME_UP);
    	return handled;
    }

    public void handleKeyEvent(KeyEvent event) {
    	//System.out.println("KeyDown: " + keyCode);

    	final int keyCode = event.getKeyCode();
    	final boolean pressedBack = (keyCode == KeyEvent.KEYCODE_BACK);

    	if (pressedBack) {
			if (!nativeHandleBack(event.getAction() == KeyEvent.ACTION_DOWN)) {
				// End game here
				if (event.getAction() == KeyEvent.ACTION_DOWN)
					finish();
			}
			return;
		}

    	if (event.getAction() == KeyEvent.ACTION_DOWN)
    		nativeOnKeyDown(keyCode);
    	else if (event.getAction() == KeyEvent.ACTION_UP)
        	nativeOnKeyUp(keyCode);
    }

    //private int _primaryPointerIdDown = -1; 
    @Override
    public boolean onTouchEvent(MotionEvent e) {
    	//System.out.println("Adding touch event " + e.getActionMasked());
    	_touchEvents.add(MotionEvent.obtainNoHistory(e));
    	return super.onTouchEvent(e);
    }

    public void handleTouchEvent(MotionEvent e) {
    	//printMemUsage(this);
    	
    	int fullAction = e.getAction();
    	int action = (fullAction & MotionEvent.ACTION_MASK);
    	//int pointerIndex = (fullAction & MotionEvent.ACTION_POINTER_INDEX_MASK) >> MotionEvent.ACTION_POINTER_INDEX_SHIFT;
    	int pointerIndex = (fullAction & MotionEvent.ACTION_POINTER_ID_MASK) >> MotionEvent.ACTION_POINTER_ID_SHIFT;
    	int pointerId = e.getPointerId(pointerIndex);
    	//System.out.println("Pointers: " + pointerId + ", " + pointerId222);
		float x = e.getX(pointerIndex) * invScale;
		float y = e.getY(pointerIndex) * invScale;

    	switch (action) {
    	case MotionEvent.ACTION_DOWN: {
    		//System.err.println("D: " + pointerId + ": " + x + ", " + y);
    		nativeMouseDown(pointerId, 1, x, y);
    		break;
    	}
    	case MotionEvent.ACTION_POINTER_DOWN: {
    		//System.err.println("d: " + pointerId + ": " + x + ", " + y);
			nativeMouseDown(pointerId, 1, x, y);
			break;
        }
    	case MotionEvent.ACTION_UP: {
    		//System.err.println("U: " + pointerId + ": " + x + ", " + y);
			nativeMouseUp(pointerId, 1, x, y);
			break;
        }
    	case MotionEvent.ACTION_POINTER_UP: {
    		//System.err.println("u: " + pointerId + ": " + x + ", " + y);
			nativeMouseUp(pointerId, 1, x, y);
			break;
    	}
		case MotionEvent.ACTION_MOVE: {
    		int pcount = e.getPointerCount();
    		for (int i = 0; i < pcount; ++i) {
    			int pp = e.getPointerId(i); // @attn wtf?																																
    			float xx = e.getX(i) * invScale;
    			float yy = e.getY(i) * invScale; 
        		//	System.err.println("   " + p + " @ " + x + ", " + y);
    			nativeMouseMove(pp, xx, yy);
    		}
    		break;
    	}
    	}
    	
    	//return super.onTouchEvent(e);
    }
    
    static private void printMemUsage(Context context)
    {
    	final String TAG = "MEMdbg";
    	
    	MemoryInfo mi = new MemoryInfo();
    	ActivityManager activityManager = (ActivityManager) context.getSystemService(ACTIVITY_SERVICE);
    	activityManager.getMemoryInfo(mi);
    	long availableKbs = mi.availMem / 1024;
    	Log.i(TAG, "" + availableKbs);
    }

    static public void saveScreenshot(String filename, int w, int h, int[] pixels) {
    	Bitmap bitmap = Bitmap.createBitmap(pixels, w, h, Bitmap.Config.ARGB_8888);

    	//System.out.println("Save screenshot: " + filename);
    	
    	try {
	        FileOutputStream fos = new FileOutputStream(filename);
	        bitmap.compress(CompressFormat.JPEG, 85, fos);
	    	//System.out.println("Compression completed!");

	        try {
                fos.flush();
	        } catch (IOException e) {
                e.printStackTrace();
	        }

	        try {
                fos.close();
	        } catch (IOException e) {
                e.printStackTrace();
	        }
        }
        catch (FileNotFoundException e) {
        	System.err.println("Couldn't create file: " + filename);
            e.printStackTrace();
        }
    }

    public int[] getImageData(String filename) {
    	AssetManager assets = getAssets();

        try {
        	/*String[] filenames = */assets.list("images");
        } catch (IOException e) {
        	System.err.println("getImageData: Could not list directory");
        	return null;
        }

        InputStream inputStream = null;
        try {
        	inputStream = assets.open(filename);
        } catch (IOException e) {
        	System.err.println("getImageData: Could not open image " + filename);
        	return null;
        }

        Bitmap bm = BitmapFactory.decodeStream(inputStream);
        int w = bm.getWidth();
        int h = bm.getHeight();

        int[] pixels = new int[w * h + 2];
        pixels[0] = w;
        pixels[1] = h;
        bm.getPixels(pixels, 2, w, 0, 0, w, h);

        return pixels;
    }

    public byte[] getFileDataBytes(String filename) {
    	AssetManager assets = getAssets();

    	BufferedInputStream bis;
    	try {
        	InputStream is = assets.open(filename);
        	bis = new BufferedInputStream(is);
        } catch (IOException e) {
        	e.printStackTrace();
        	return null;
        }

    	ByteArrayOutputStream s = new ByteArrayOutputStream(4096);
    	byte[] tmp = new byte[1024];
    	
    	try {
	    	while (true) {
	    		int count = bis.read(tmp);
	    		if (count <= 0) break;
	    		s.write(tmp, 0, count);
	    	}
    	} catch (IOException e) {
    	} finally {
    		try { bis.close(); }
    		catch (IOException e) {}
    	}

    	return s.toByteArray();
    }
    
    public int getScreenWidth() {
    	Display display = ((WindowManager)this.getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay();
    	int out = Math.max(display.getWidth(), display.getHeight());
    	//System.out.println("getwidth: " + out);
    	return out;
    }
    
    public int getScreenHeight() {
    	Display display = ((WindowManager)this.getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay();
    	int out = Math.min(display.getWidth(), display.getHeight());
    	//System.out.println("getheight: " + out);
    	return out;
    }

    public float getPixelsPerMillimeter() {
   	    DisplayMetrics metrics = new DisplayMetrics();
   	    getWindowManager().getDefaultDisplay().getMetrics(metrics);
   	    return (metrics.xdpi + metrics.ydpi) * 0.5f / 25.4f;
    }

    public int checkLicense() { return LicenseCodes.LICENSE_OK; }
    
    public String getDateString(int s) {
    	return DateFormat.format(new Date(s * 1000L));
    }

    public boolean hasBuyButtonWhenInvalidLicense() { return true; }
    
    public void postScreenshotToFacebook(String filename, int w, int h, int[] pixels) {
    	return;
    }
    
    @Override
    protected void onActivityResult(int requestCode, int resultCode,
                                    Intent data) {

    	if (requestCode == DialogDefinitions.DIALOG_MAINMENU_OPTIONS) {
    		_userInputStatus = 1;
    	}
    }
    
    public void quit() {
		runOnUiThread(new Runnable() {
			public void run() { finish(); } });
    }

    public void displayDialog(int dialogId) {

    	if (dialogId == DialogDefinitions.DIALOG_CREATE_NEW_WORLD) {
    		chooseDialog(R.layout.create_new_world,
    			new int[] { R.id.editText_worldName,
    						R.id.editText_worldSeed,
    						R.id.button_gameMode},
    			false, // Don't prevent back key
    			R.id.button_createworld_create,
    			R.id.button_createworld_cancel
    		); 
    	} else if (dialogId == DialogDefinitions.DIALOG_MAINMENU_OPTIONS) { 
    		Intent intent = new Intent(this, MainMenuOptionsActivity.class);
    		intent.putExtra("preferenceId", R.xml.preferences);
    		startActivityForResult(intent, dialogId);
    	} else if (dialogId == DialogDefinitions.DIALOG_RENAME_MP_WORLD) {
    		chooseDialog(R.layout.rename_mp_world,
    			new int[] { R.id.editText_worldNameRename },
    			false
        	);
    	}
    }

    void chooseDialog(final int layoutId, final int[] viewIds) {
    	chooseDialog(layoutId, viewIds, true);
    }
    void chooseDialog(final int layoutId, final int[] viewIds, final boolean hasCancelButton) {
    	chooseDialog(layoutId, viewIds, hasCancelButton, true);
    }
    void chooseDialog(final int layoutId, final int[] viewIds, final boolean hasCancelButton, final boolean preventBackKey) {
    	chooseDialog(layoutId, viewIds, preventBackKey, 0, hasCancelButton? 0 : -1);
    }
    void chooseDialog(final int layoutId, final int[] viewIds, final boolean preventBackKey, final int okButtonId, final int cancelButtonId) {
    	_userInputValues.clear();

    	runOnUiThread(new Runnable() {
    	    public void run() {
    	    	createAlertDialog(okButtonId==0, cancelButtonId==0, preventBackKey);
    	        LayoutInflater li = LayoutInflater.from(MainActivity.this);
    	        
    	        try {
                    View view = li.inflate(layoutId, null);
                    if (okButtonId != 0 && okButtonId != -1) {
                    	View b = view.findViewById(okButtonId);
                    	if (b != null)
                    		b.setOnClickListener(new View.OnClickListener()
                    			{ public void onClick(View v) { if (mDialog != null) mDialog.dismiss(); onDialogCompleted(); }});
                    }
                    if (cancelButtonId != 0 && cancelButtonId != -1) {
                    	View b = view.findViewById(cancelButtonId);
                    	if (b != null)
                    		b.setOnClickListener(new View.OnClickListener()
                    			{ public void onClick(View v) { if (mDialog != null) mDialog.cancel(); onDialogCanceled(); }});
                    }

                    //mDialog.setO
                    MainActivity.this.mDialog.setView(view);

                    if (viewIds != null)
                    	for (int viewId : viewIds) {
                    		View v = view.findViewById(viewId);
                    		if (v instanceof StringValue)
                    			_userInputValues.add( (StringValue) v );
                    		else if (v instanceof TextView)
                    			_userInputValues.add(new TextViewReader((TextView)v));
                    	}

    	        } catch (Error e) {
    	        	e.printStackTrace();
    	        }

    	        MainActivity.this.mDialog.show();
    	        MainActivity.this.mDialog.getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
    	        MainActivity.this.mDialog.getWindow().setLayout(LayoutParams.FILL_PARENT, LayoutParams.FILL_PARENT);
    	        //MainActivity.this.getWindow().setLayout(LayoutParams.FILL_PARENT, LayoutParams.FILL_PARENT);
    	    }
    	});
    }
    
    public void tick() {}

    public String[] getOptionStrings() {
    	SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this); 
    	Map<String, ?> m = prefs.getAll();

    	String[] tmpOut = new String[m.size() * 2];

    	int n = 0;
    	for (Map.Entry<String, ?> e : m.entrySet()) {
    		String key   = e.getKey();
    		String value = e.getValue().toString();
    		//
    		// Feel free to modify key or value!
    		//
    		// This can be used to correct differences between the different
    		// platforms, such as Android not supporting floating point
    		// ranges for Sliders/SeekBars: {0..100} - TRANSFORM -> {0..1}
    		//
    		if (key.equals(MainMenuOptionsActivity.Internal_Game_DifficultyPeaceful)) {
    			key = MainMenuOptionsActivity.Game_DifficultyLevel;
    			value = ((Boolean) e.getValue()) ? "0" : "2";
    		}
    		
    		try {
    			if (key.equals(MainMenuOptionsActivity.Controls_Sensitivity))
    				value = new Double( 0.01 * Integer.parseInt(value) ).toString();
    		} catch (Exception exc) {}

    		tmpOut[n++] = key;
    		tmpOut[n++] = value;
//    		System.out.println("Key: " + e.getKey());
//    		System.out.println("Val: " + e.getValue().toString() + "\n");
    	}

    	// Copy over the enabled preferences
    	String[] out = new String[n];
    	for (int i = 0; i < n; ++i)
    		out[i] = tmpOut[i];
    	
    	return out;
    }

    public void buyGame() {}

    public String getPlatformStringVar(int id) {
    	if (id == 0) return android.os.Build.MODEL;
    	return null;
    }

    public boolean isNetworkEnabled(boolean onlyWifiAllowed) {
    	return true;
    	/*
	    ConnectivityManager cm = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
	    NetworkInfo info = onlyWifiAllowed? cm.getNetworkInfo(ConnectivityManager.TYPE_WIFI)
	    								:	cm.getActiveNetworkInfo();
	  //(info.getState() == NetworkInfo.State.CONNECTED || info.getState() == NetworkInfo.State.CONNECTING));
	    return (info != null && info.isConnectedOrConnecting());
    	 */
    }

    private Bundle data;
    private int _userInputStatus = -1;
    private String[] _userInputText = null;
    private ArrayList<StringValue> _userInputValues = new ArrayList<StringValue>();
    public void initiateUserInput(int id) {
    	_userInputText = null;
    	_userInputStatus = -1;
    }
    public int getUserInputStatus() { return _userInputStatus; }
    public String[] getUserInputString() { return _userInputText; }
    
	private SoundPlayer _soundPlayer;
    public void playSound(String s, float volume, float rate) {
    	_soundPlayer.play(s, volume, rate);
    }

    private AlertDialog mDialog;
    private final DateFormat DateFormat = new SimpleDateFormat();
//    public EditText mTextInputWidget;

    private void onDialogCanceled() {
    	_userInputStatus = 0;
    }

    private void onDialogCompleted() {
	    int size = _userInputValues.size(); 
	    _userInputText = new String[size];
	    for (int i = 0; i < size; ++i) {
	    	_userInputText[i] = _userInputValues.get(i).getStringValue(); 
	    }
	    for (String s : _userInputText) System.out.println("js: " + s);

	    _userInputStatus = 1;
	}
    
    
    protected void onStart() {
    	//System.out.println("onStart");
    	super.onStart();
    }
    
    protected void onResume() {
    	//System.out.println("onResume");
    	super.onResume();
        _glView.onResume();
    }

    protected void onPause() {
    	//System.out.println("onPause");
    	super.onPause();
        _glView.onPause();
    }

    protected void onStop() {
    	//System.out.println("onStop");
    	super.onStop();
    }
    protected void onDestroy() {
    	//System.out.println("onDestroy");

    	nativeUnregisterThis();
    	super.onDestroy();
    	nativeOnDestroy();
    }

    protected boolean isDemo() { return false; }
    
    //
    // Native interface
    //
    native void nativeRegisterThis();
    native void nativeUnregisterThis();
    native static void nativeOnCreate();
    native static void nativeOnDestroy();
    native static void nativeOnKeyDown(int key);
    native static void nativeOnKeyUp(int key);
    native static boolean nativeHandleBack(boolean isDown);
    native static void nativeMouseDown(int pointerId, int buttonId, float x, float y);
    native static void nativeMouseUp(int pointerId, int buttonId, float x, float y);
    native static void nativeMouseMove(int pointerId, float x, float y);

    static {
        System.loadLibrary("minecraftpe");
    }

    public void clearPendingEvents() {
		_touchEvents.clear();
		_keyEvents.clear();
    }
	public void processPendingEvents() {
		int i = 0;
		while (i < _touchEvents.size())
			handleTouchEvent(_touchEvents.get(i++));
		i = 0;
		while (i < _keyEvents.size())
			handleKeyEvent(_keyEvents.get(i++));

		clearPendingEvents();
	}
}

// see client/gui/screens/DialogDefinitions.h
class DialogDefinitions {
	static final int DIALOG_CREATE_NEW_WORLD = 1;
	static final int DIALOG_NEW_CHAT_MESSAGE = 2;
	static final int DIALOG_MAINMENU_OPTIONS = 3;
	static final int DIALOG_RENAME_MP_WORLD = 4;
}


class GLView extends GLSurfaceView {
	public GLView(Context context, MainActivity activity) {
        super(context);
        _activity = activity;
        _renderer = new GLRenderer(activity);
        
    }

	@Override
	public void onPause() { _renderer.paused = true; }
	@Override
	public void onResume() { _renderer.paused = false; }
	
	public void commit() {
		setRenderer(_renderer);
	}
	
    @Override
    public void surfaceCreated(android.view.SurfaceHolder holder) {
//    	holder.setFixedSize((int)(_activity.getScreenWidth() * _activity.invScale),
//    						(int)(_activity.getScreenHeight() * _activity.invScale));
        System.out.println("w,h: " + _activity.getScreenWidth() + "," + _activity.getScreenHeight());
    	super.surfaceCreated(holder);
    }

//    public boolean onTouchEvent(final MotionEvent event) {
//        if (event.getAction() == MotionEvent.ACTION_DOWN) {
//            nativePause();
//        }
//        return true;
//    }

    private GLRenderer _renderer;
	private MainActivity _activity;
}

class GLRenderer implements GLSurfaceView.Renderer {
	private MainActivity _activity;
	
	public GLRenderer(MainActivity a) {
		_activity = a;
	}
	
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        nativeOnSurfaceCreated();
    }

    public void onSurfaceChanged(GL10 gl, int w, int h) {
    	if (h > w) {
    		System.out.println("surfchanged ERROR. dimensions: " + w + ", " + h);
    		nativeOnSurfaceChanged(h, w);
    	}
    	else
    		nativeOnSurfaceChanged(w, h);
    }

    public void onDrawFrame(GL10 gl) {
    	if (!paused) {
    		_activity.processPendingEvents();
    		nativeUpdate();
    	} else {
    		_activity.clearPendingEvents();
    	}
    }

    public boolean paused = false;
    
    private static native void nativeOnSurfaceCreated();
    private static native void nativeOnSurfaceChanged(int w, int h);
    private static native void nativeUpdate();
    //private static native void nativeDone();
}

class TextViewReader implements StringValue {
	public TextViewReader(TextView view) {
		_view = view;
	}
	public String getStringValue() {
		return _view.getText().toString();
	}
	private TextView _view;
}
