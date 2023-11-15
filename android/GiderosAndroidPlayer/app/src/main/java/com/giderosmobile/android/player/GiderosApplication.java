package com.giderosmobile.android.player;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.BufferedOutputStream;
import java.lang.reflect.Method;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;
import java.util.Locale;
import java.util.Timer;
import java.util.TimerTask;

import dalvik.system.DexClassLoader;
import android.app.Activity;
import android.app.UiModeManager;
import android.content.ActivityNotFoundException;
import android.content.ClipboardManager;
import android.content.ClipData;
import android.content.ClipData.Item;
import android.content.ClipDescription;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.res.AssetFileDescriptor;
import android.content.res.AssetManager;
import android.content.res.Configuration;
import android.graphics.Color;
import android.media.AudioManager;
import android.net.Uri;
import android.opengl.GLSurfaceView;
import android.os.Environment;
import android.os.Vibrator;
import android.os.BatteryManager;
import android.text.Editable;
import android.text.Selection;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.KeyEvent;
import android.view.Surface;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowInsetsController;
import android.view.WindowManager;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.ExtractedText;
import android.view.inputmethod.ExtractedTextRequest;
import android.view.inputmethod.InputMethodManager;
import android.view.inputmethod.InputConnection;
import android.text.InputType;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.FrameLayout;
import android.widget.ListView;
import android.widget.TextView;
import android.view.DisplayCutout;
import android.view.WindowInsets.Type;
import android.os.Build;

import com.giderosmobile.android.GiderosSettings;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class GiderosApplication
{
	public Object lock = new Object();
	
	public ZipResourceFile mainFile;
	public ZipResourceFile patchFile;
	
	static private GiderosApplication instance_;	
	static public GiderosApplication getInstance()
	{
		return instance_;
	}
	
	private Accelerometer accelerometer_;
	private boolean isAccelerometerStarted_ = false;

	private Gyroscope gyroscope_;
	private boolean isGyroscopeStarted_ = false;

	private Geolocation geolocation_;
	private boolean isLocationStarted_ = false;
	private boolean isHeadingStarted_ = false;

	private boolean isForeground_ = false;

	private boolean isSurfaceCreated_ = false;

	private ArrayList<Integer> eventQueue_ = new ArrayList<Integer>();
	private static final int PAUSE = 0;
	private static final int RESUME = 1;
	private static final int STOP = 2;
	private static final int START = 3;
	
	private ListView projectList;
	
	private GGMediaPlayerManager mediaPlayerManager_;
	
	//private AudioDevice audioDevice_;
	
	private ArrayList < Class < ? >>	sAvailableClasses = new ArrayList < Class < ? >> ();
	
	private static Class < ? > findClass ( String className ) {
		
		Class < ? > theClass = null;
		try {

			theClass = Class.forName ( className );
		} catch ( Throwable e ) {

		}
		
		return theClass;
	}
	
	private static Object executeMethod ( Class < ? > theClass, Object theInstance, String methodName, Class < ? > [] parameterTypes, Object [] parameterValues ) {
		
		Object result = null;
		if ( theClass != null ) {
			
			try {

				Method theMethod = theClass.getMethod ( methodName, parameterTypes );

				result = theMethod.invoke ( theInstance, parameterValues );
			} catch ( Throwable e ) {
				
			}			
		}
		
		return result;
	}
	
	public GiderosApplication(String[] sExternalClasses)
	{
		for ( String className : sExternalClasses )
		{
			if (className != null)
			{
				Class < ? > theClass = findClass ( className );
				if ( theClass != null ) {				
					sAvailableClasses.add ( theClass );
				}
			}
		}
		
		accelerometer_ = new Accelerometer();
		gyroscope_ = new Gyroscope();
		geolocation_ = new Geolocation();
	
		populateAllFiles();
		getDirectories();
		
		mediaPlayerManager_ = new GGMediaPlayerManager(mainFile, patchFile);

		//audioDevice_ = new AudioDevice();
		Activity activity=WeakActivityHolder.get();
		int sampleRate=0;
		if (android.os.Build.VERSION.SDK_INT >= 17)
		{
			try
			{
				AudioManager am = (AudioManager) activity.getSystemService(Context.AUDIO_SERVICE);
				String sampleRateStr = am.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE);
				sampleRate = Integer.parseInt(sampleRateStr);
			}
			catch (Exception e)
			{
			}
		}
		if (sampleRate == 0) sampleRate = 44100; // Use a default value if property not found
		GiderosApplication.nativeOpenALSetup(sampleRate);
				
		synchronized (lock)
		{
			GiderosApplication.nativeCreate(allfiles_ == null,activity);
			GiderosApplication.nativeSetDirectories(externalDir_, internalDir_, cacheDir_);
			if (allfiles_ != null)
				GiderosApplication.nativeSetFileSystem(allfiles_);
		}	
		
		loadLpkPlugins();
	}
	
	public void loadLpkPlugins()
	{
		Activity activity=WeakActivityHolder.get();
		AssetManager assetManager = activity.getAssets();
		try {
			String[] dexs = assetManager.list("dex");
			String libraryPath = activity.getApplicationContext().getApplicationInfo().nativeLibraryDir;
			Log.v("LPK","Looking for LPK plugins");
			for (String dexn:dexs)
			{
				String dex=dexn.substring(0, dexn.indexOf("."));
				Log.v("LPK","Found "+dexn+" ("+dex+")");
				File dexInternalStoragePath = new File(activity.getDir("dex", Context.MODE_PRIVATE),dex+".dex");
				if (!dexInternalStoragePath.exists())
				{
					//Copy dex to private area
			    BufferedInputStream bis = null;
				OutputStream dexWriter = null;

				  final int BUF_SIZE = 8 * 1024;
				  try {
				      bis = new BufferedInputStream(assetManager.open("dex/"+dex+".dex"));
				      dexWriter = new BufferedOutputStream(new FileOutputStream(dexInternalStoragePath));
				      byte[] buf = new byte[BUF_SIZE];
				      int len;
				      while((len = bis.read(buf, 0, BUF_SIZE)) > 0) {
				          dexWriter.write(buf, 0, len);
				      }
				      dexWriter.close();
				      bis.close();				      
				  } catch (Exception ge)
				  {
						ge.printStackTrace();
					  dexInternalStoragePath.delete();
				  }
				}
				if (dexInternalStoragePath.exists())
				{
					//Load plugin from dex
					  // Internal storage where the DexClassLoader writes the optimized dex file to
					  final File optimizedDexOutputPath = activity.getDir("optdex", Context.MODE_PRIVATE);

					  DexClassLoader cl = new DexClassLoader(dexInternalStoragePath.getAbsolutePath(),
					                                         optimizedDexOutputPath.getAbsolutePath(),
					                                         libraryPath,
					                                         activity.getClassLoader());
					  try {
						Class<?> k=cl.loadClass("com.giderosmobile.android.plugins."+dex+".Loader");
						try {
							k.newInstance();
							Log.v("LPK","Loaded "+dex+" :"+k.getName());
							try {
								Method mtd = k.getMethod("onLoad", Activity.class);
								mtd.invoke(null, activity);
								Log.v("LPK","Initialized "+dex+" :"+k.getName());
							} catch (NoSuchMethodException le) {
							} catch (Exception le) {
								le.printStackTrace();
							}
						} catch (InstantiationException e) {
							e.printStackTrace();
						} catch (IllegalAccessException e) {
							e.printStackTrace();
						}
					} catch (ClassNotFoundException e) {
						e.printStackTrace();
					}
				}
			}
		} catch (IOException e) {
			e.printStackTrace();
		} catch (Error e) {
			e.printStackTrace();
		}
	}

	private String allfiles_ = null;

	private void populateAllFiles()
	{
		allfiles_ = null;
		
		AssetManager assetManager = WeakActivityHolder.get().getAssets();

		ArrayList<String> lines = new ArrayList<String>();

		try
		{
			InputStream in = assetManager.open("assets/allfiles.txt");
			BufferedReader br = new BufferedReader(new InputStreamReader(in));
			while (true)
			{
				String line = br.readLine();
				if (line == null)
					break;
				lines.add(line);
			}
			in.close();
		} catch (IOException e)
		{
			loadProjects();
			Logger.log("player mode");
			return;
		}		
				
		StringBuilder sb = new StringBuilder();

		ApplicationInfo applicationInfo = WeakActivityHolder.get().getApplicationInfo();
		sb.append(applicationInfo.sourceDir).append("|");

		lines.add("properties.bin*");
		lines.add("luafiles.txt*");
					
		String mainPath = null;
		String patchPath = null;
		mainFile = null;
		patchFile = null;
		try
		{
			if (Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED))
			{
				Activity activity = WeakActivityHolder.get();

				String packageName = activity.getPackageName();
				String root = Environment.getExternalStorageDirectory().getAbsolutePath();
				String expPath = root + "/Android/obb/" + packageName;

				int versionCode = activity.getPackageManager().getPackageInfo(activity.getPackageName(), 0).versionCode;

				mainPath = expPath + "/" + "main." + versionCode + "." + packageName + ".obb";
				if ((new File(mainPath)).isFile())
					mainFile = new ZipResourceFile(mainPath);

				patchPath = expPath + "/" + "patch." + versionCode + "." + packageName + ".obb";
				if ((new File(patchPath)).isFile())
					patchFile = new ZipResourceFile(patchPath);
			}
		} catch (Exception e)
		{
		}
		
		if (mainFile != null)
			sb.append(mainPath);
		sb.append("|");			

		if (patchFile != null)
			sb.append(patchPath);
		sb.append("|");			

		for (int i = 0; i < lines.size(); ++i)
		{
			String line = lines.get(i);				
			
			String suffix = "";
			
			if (line.endsWith("*"))
			{
				line = line.substring(0, line.length() - 1);
				suffix = ".jet";
			}
			
			try
			{
				int zipFile = 0;
				AssetFileDescriptor fd = null;

				if (patchFile != null)
				{
					fd = patchFile.getAssetFileDescriptor(line + suffix);
					zipFile = 2;
				}

				if (fd == null)
					if (mainFile != null)
					{
						fd = mainFile.getAssetFileDescriptor(line + suffix);
						zipFile = 1;
					}

				if (fd == null)
				{
					fd = assetManager.openFd("assets/" + line + suffix);
					zipFile = 0;
				}

				if (fd != null)
				{
					sb.append(line).append("|").append(zipFile).append("|").append(fd.getStartOffset()).append("|").append(fd.getLength()).append("|");
					fd.close();
				}
			} catch (IOException e)
			{
				Logger.log(e.toString());
			}
		}

		sb.deleteCharAt(sb.length() - 1);
		
		allfiles_ = sb.toString();
	}
	
	String externalDir_, internalDir_, cacheDir_;
	private void getDirectories()
	{
		boolean mExternalStorageAvailable = false;
		boolean mExternalStorageWriteable = false;
		String state = Environment.getExternalStorageState();

		if (Environment.MEDIA_MOUNTED.equals(state))
		{
			// We can read and write the media
			mExternalStorageAvailable = mExternalStorageWriteable = true;
		} else if (Environment.MEDIA_MOUNTED_READ_ONLY.equals(state))
		{
			// We can only read the media
			mExternalStorageAvailable = true;
			mExternalStorageWriteable = false;
		} else
		{
			// Something else is wrong. It may be one of many other states, but
			// all we need
			// to know is we can neither read nor write
			mExternalStorageAvailable = mExternalStorageWriteable = false;
		}

		File extdir = WeakActivityHolder.get().getExternalFilesDir(null);
		internalDir_ = WeakActivityHolder.get().getFilesDir().getAbsolutePath();
		externalDir_ = (extdir==null)?internalDir_:extdir.getAbsolutePath();
		cacheDir_ = WeakActivityHolder.get().getCacheDir().getAbsolutePath();

		Logger.log("externalDir: " + externalDir_);
		Logger.log("internalDir: " + internalDir_);
		Logger.log("cacheDir: " + cacheDir_);
	}
	
	private void loadProjects(){
		projectList = new ListView(WeakActivityHolder.get());
		TextView text = new TextView(WeakActivityHolder.get());
		text.setText("Gideros Projects");
		text.setTextColor(Color.BLACK);
		text.setTextSize(25);
		text.setBackgroundColor(Color.WHITE);
		projectList.addHeaderView(text);
		ArrayAdapter<String> modeAdapter = new ArrayAdapter<String>(WeakActivityHolder.get(), android.R.layout.simple_list_item_1, android.R.id.text1, traverse(new File(WeakActivityHolder.get().getExternalFilesDir(null),"gideros"))){
			@Override
	        public View getView(int position, View convertView, ViewGroup parent) {
	            View view =super.getView(position, convertView, parent);

	            TextView textView=(TextView) view.findViewById(android.R.id.text1);
	            textView.setTextColor(Color.BLACK);
	            textView.setBackgroundColor(Color.WHITE);

	            return view;
	        }
		};
		projectList.setAdapter(modeAdapter);
		projectList.setOnItemClickListener(new OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
            	TextView textView=(TextView) view.findViewById(android.R.id.text1);
            	if(textView != null){
            		projectList.setVisibility(View.GONE);
            		nativeOpenProject((String) textView.getText());
            	}
            }
        });
		projectList.setVisibility(View.GONE);
		FrameLayout layout = (FrameLayout)WeakActivityHolder.get().getWindow().getDecorView();
		layout.addView(projectList);
	}
	
	public List<String> traverse (File dir) {
		Logger.log("Checking: " + dir.getAbsolutePath());
		List<String> projects = new ArrayList<String>();
	    if (dir.exists()) {
	        File[] files = dir.listFiles();
            if(files != null){
                for (int i = 0; i < files.length; ++i) {
                    File file = files[i];
                    if (file.isDirectory()) {
                        Logger.log("Found: " + file.getName());
                        projects.add(file.getName());
                    }
                }
            }
	    }
	    return projects;
	} 
	
	
	private static SurfaceView mGLView_;
	private boolean needRender=false;
	private Runnable onDemandRender=null;
	
	public void enableOnDemand(boolean en)
	{
		if (en&&(onDemandRender==null)) {
			onDemandRender=new Runnable() {
				@Override
				public void run() {
					onDrawFrame(true);
					if (onDemandRender!=null) {
						if (needRender) {
							((GLSurfaceView)mGLView_).requestRender();
							needRender=false;
						}
						else
							((GLSurfaceView)mGLView_).queueEvent(this);
					}
				}
			};
			((GLSurfaceView)mGLView_).queueEvent(onDemandRender);
			((GLSurfaceView)mGLView_).setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
		}
		else if ((!en)&&(onDemandRender!=null)) {
			onDemandRender=null;
			((GLSurfaceView)mGLView_).setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);
		}				
	}

	public void requestDraw()
	{
		needRender=true;
	}
	
	static public void onCreate(String[] externalClasses, SurfaceView mGLView)
	{
		mGLView_=mGLView;
		instance_ = new GiderosApplication(externalClasses);
		setKeyboardVisibility(false);
		for ( Class < ? > theClass : instance_.sAvailableClasses ) {
			
			executeMethod ( theClass, null, "onCreate", new Class < ? > [] { Activity.class }, new Object [] { WeakActivityHolder.get() });
		}
	}
	static public int[] getSafeArea() {
		int[] insets=new int[4];
		if (GiderosSettings.notchReady&&(Build.VERSION.SDK_INT >= Build.VERSION_CODES.P)) {
			DisplayCutout cutout = mGLView_.getRootWindowInsets().getDisplayCutout();
			if (cutout != null) {
				insets[0]=cutout.getSafeInsetLeft();
				insets[1]=cutout.getSafeInsetRight();
				insets[2]=cutout.getSafeInsetTop();
				insets[3]=cutout.getSafeInsetBottom();
			}
		}
		return insets;
	}
		
	static public void onDestroy()
	{
		for ( Class < ? > theClass : instance_.sAvailableClasses ) {

			executeMethod ( theClass, null, "onDestroy", new Class < ? > [] { }, new Object [] { });
		}	

		synchronized (instance_.lock)
		{
			GiderosApplication.nativeDestroy();
		}

		instance_ = null;
	}

	public void onStart()
	{
		oculusStart();
		if (isSurfaceCreated_ == true) {
			synchronized (eventQueue_) {
				eventQueue_.add(START);
			}
		}
		
		for ( Class < ? > theClass : sAvailableClasses ) {

			executeMethod ( theClass, null, "onStart", new Class < ? > [] { }, new Object [] { });
		}		
	}
	
	public void onRestart()
	{
		for ( Class < ? > theClass : sAvailableClasses ) {

			executeMethod ( theClass, null, "onRestart", new Class < ? > [] { }, new Object [] { });
		}
	}
	
	public void onStop()
	{
		oculusStop();
		for ( Class < ? > theClass : sAvailableClasses ) {

			executeMethod ( theClass, null, "onStop", new Class < ? > [] { }, new Object [] { });
		}

		if (isSurfaceCreated_ == true) {
			synchronized (eventQueue_) {
				nativeStop();
				nativeTick();
			}
		}
	}
	
	static boolean onDemandEnabled=false;
	public void onPause()
	{
		if (onDemandEnabled)
			enableOnDemand(false);
		oculusPause();
		isForeground_ = false;

		for ( Class < ? > theClass : sAvailableClasses ) {

			executeMethod ( theClass, null, "onPause", new Class < ? > [] { }, new Object [] { });
		}	

		if (isSurfaceCreated_ == true) {
			synchronized (eventQueue_) {
				nativePause();
				nativeTick();
			}
		}

		
		accelerometer_.disable();
		gyroscope_.disable();
		if (!GiderosSettings.backgroundLocation)
		{
			geolocation_.stopUpdatingLocation();
			geolocation_.stopUpdatingHeading();
		}
		mediaPlayerManager_.onPause();
		//audioDevice_.stop();
	}
	
	public void onResume()
	{
		oculusResume();
		isForeground_ = true;
		if (isAccelerometerStarted_)
			accelerometer_.enable();
		if (isGyroscopeStarted_)
			gyroscope_.enable();
		if (!GiderosSettings.backgroundLocation)
		{
			if (isLocationStarted_)
				geolocation_.startUpdatingLocation();
			if (isHeadingStarted_)
				geolocation_.startUpdatingHeading();
		}
		mediaPlayerManager_.onResume();
		//audioDevice_.start();

		for ( Class < ? > theClass : sAvailableClasses ) {

			executeMethod ( theClass, null, "onResume", new Class < ? > [] { }, new Object [] { });
		}	

		if (isSurfaceCreated_ == true) {
			synchronized (eventQueue_) {
				nativeResume();
				nativeTick();
			}
		}
		if (onDemandEnabled)
			enableOnDemand(true);
	}

	public void onLowMemory()
	{
		synchronized (lock)
		{
			nativeLowMemory();
		}
	}
	
	public void onHandleOpenUrl(String url)
	{
		nativeHandleOpenUrl(url);
	}
	
	public void onActivityResult(int requestCode, int resultCode, Intent data)
	{
		for ( Class < ? > theClass : sAvailableClasses ) {

			executeMethod ( theClass, null, "onActivityResult", new Class < ? > [] { java.lang.Integer.TYPE, java.lang.Integer.TYPE, Intent.class }, new Object [] { new Integer ( requestCode ), new Integer ( resultCode ), data });
		}	
	}

	public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults)
	{
		for ( Class < ? > theClass : sAvailableClasses )
		{
			executeMethod ( theClass, null, "onRequestPermissionsResult", 
					new Class < ? > [] { Integer.TYPE, String[].class, int[].class }, 
					new Object [] { new Integer ( requestCode ), permissions, grantResults });
		}
	}
	
	public void onSurfaceCreated(Surface surface)
	{
		synchronized (lock)
		{
			GiderosApplication.nativeSurfaceCreated(surface);
			isSurfaceCreated_ = true;
		}		
	}
	
	public void onSurfaceChanged(int w, int h,Surface surface)
	{
		synchronized (lock)
		{
			GiderosApplication.nativeSurfaceChanged(w, h, getRotation(w,h),surface);
		}	
	}

	public void onSurfaceDestroyed()
	{
		synchronized (lock)
		{
			GiderosApplication.nativeSurfaceDestroyed();
		}	
	}

	static private void sleep(long nsec)
	{
		try {
			Thread.sleep(nsec / 1000000, (int)(nsec % 1000000));
		} catch (InterruptedException e) {
		}
	}

	private long startTime = System.nanoTime();

	public void onDrawFrame(boolean tick)
	{		
		long target = 1000000000L / (long)fps_;
		if ((onDemandRender==null)||tick) {		
			long currentTime = System.nanoTime();
			long dt = currentTime - startTime;
			startTime = currentTime;
			if (dt < 0)
				dt = 0;
			if (dt < target)
			{
				sleep(target - dt);
				startTime += target - dt;
			}
		}
	    
		synchronized (lock) {
			synchronized (eventQueue_) {
				while (!eventQueue_.isEmpty()) {
					int event = eventQueue_.remove(0);
					switch (event) {
					case PAUSE:
						GiderosApplication.nativePause();
						break;
					case RESUME:
						GiderosApplication.nativeResume();
						break;
					case START:
						GiderosApplication.nativeStart();
						break;
					case STOP:
						GiderosApplication.nativeStop();
						break;
					}
				}
				eventQueue_.notify();
			}

			GiderosApplication.nativeDrawFrame(tick);			
		}
		
		if ((onDemandRender!=null)&&!tick)
			((GLSurfaceView)mGLView_).queueEvent(onDemandRender);
	}

	public void onMouseWheel(int x,int y,int button,float amount)
	{
		GiderosApplication.nativeMouseWheel(x,y,button,amount);
	}

	public void onTouchesBegin(int size, int[] id, int[] x, int[] y, float[] pressure, int actionIndex)
	{
		if(!isRunning()){
			if(projectList != null && projectList.getVisibility() == View.GONE){
				projectList.setVisibility(View.VISIBLE);
			}
		}
		GiderosApplication.nativeTouchesBegin(size, id, x, y, pressure, actionIndex);
	}

	public void onTouchesMove(int size, int[] id, int[] x, int[] y, float[] pressure)
	{	
		GiderosApplication.nativeTouchesMove(size, id, x, y, pressure);
	}

	public void onTouchesEnd(int size, int[] id, int[] x, int[] y, float[] pressure, int actionIndex)
	{
		GiderosApplication.nativeTouchesEnd(size, id, x, y, pressure, actionIndex);
	}

	public void onTouchesCancel(int size, int[] id, int[] x, int[] y, float[] pressure)
	{
		GiderosApplication.nativeTouchesCancel(size, id, x, y, pressure);
	}	
	
	public boolean onKeyDown(int keyCode, KeyEvent event)
	{
		if(projectList != null && projectList.getVisibility() == View.VISIBLE){
			projectList.setVisibility(View.GONE);
			return true;
		}
		boolean handled = nativeKeyDown(keyCode, event.getRepeatCount());
		if (event.getUnicodeChar()>0)
			nativeKeyChar(Character.toString((char)event.getUnicodeChar()));
		if(keyCode == KeyEvent.KEYCODE_VOLUME_DOWN || keyCode == KeyEvent.KEYCODE_VOLUME_MUTE || keyCode == KeyEvent.KEYCODE_VOLUME_UP || keyCode == KeyEvent.KEYCODE_POWER){
			return false;
		}
		return handled;
	}
	
	public boolean onKeyUp(int keyCode, KeyEvent event)
	{
		return nativeKeyUp(keyCode, event.getRepeatCount());
	}
	
	public boolean onCheckIsTextEditor ()
	{
		return true;
	}
	static String tisLabel="";
	static String tisHint="";
	static String tisActionLabel="";
	static int tisType=InputType.TYPE_NULL;
	static Editable tisEditable=null;
	static int tisSelStart=-1;
	static int tisSelEnd=-1;
	static int tisInitCapsMode=0;
	static String tisBuffer="";
	static int tisToken=-1;
	static String tisContext="";
	
	public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
		outAttrs.actionLabel = tisActionLabel;
		outAttrs.hintText = tisHint;
		outAttrs.initialCapsMode = tisInitCapsMode;
		outAttrs.initialSelEnd = tisSelEnd;
		outAttrs.initialSelStart = tisSelStart;
		outAttrs.label = tisLabel;
		outAttrs.imeOptions = EditorInfo.IME_ACTION_DONE;		
		outAttrs.inputType = tisType;
		
		Log.v("EBE","OCI:"+tisType+" SS:"+tisSelStart+" SE:"+tisSelEnd);
		BaseInputConnection b=new BaseInputConnection(mGLView_, tisType!=0) {
			public boolean endBatchEdit() {
				Editable e=getEditable();
				if (tisEditable==e) {
					tisSelStart=Selection.getSelectionStart(e);
					tisSelEnd=Selection.getSelectionEnd(e);
					tisBuffer=e.toString();
					//Log.v("EBE","TE:"+tisEditable+" SS:"+tisSelStart+" SE:"+tisSelEnd+" BB:"+e.toString());
					GiderosApplication app = GiderosApplication.getInstance();
					nativeTextInput(tisBuffer,tisSelStart,tisSelEnd,tisContext);
					if (tisToken!=-1) {
						Activity activity = WeakActivityHolder.get();
				    	InputMethodManager imm = (InputMethodManager)
				    			activity.getSystemService(Context.INPUT_METHOD_SERVICE);
						ExtractedText et=new ExtractedText();
						et.text=e.toString();
						et.startOffset=0; 
						et.partialStartOffset=-1;
						et.partialEndOffset=-1;
						et.selectionStart=(tisSelStart>=0)?tisSelStart-et.startOffset:-1;
						et.selectionEnd=(tisSelEnd>=0)?tisSelEnd-et.startOffset:-1;
						et.flags=0;
			    		imm.updateExtractedText(mGLView_,tisToken,et);
					}
				}
				return super.endBatchEdit();
			}
			public boolean deleteSurroundingText(int beforeLength, int afterLength)  {
				//Log.v("EBE","DST "+beforeLength+","+afterLength);
				if (tisType==0) {
					   if (beforeLength == 1 && afterLength == 0) {
					        // backspace
					        return super.sendKeyEvent(new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_DEL))
					            && super.sendKeyEvent(new KeyEvent(KeyEvent.ACTION_UP, KeyEvent.KEYCODE_DEL));
					    }
				}
				return super.deleteSurroundingText(beforeLength,afterLength);
			}
			public boolean sendKeyEvent(KeyEvent event) {
				//Log.v("EBE","SKE "+event);
				if (tisType!=0) {
					if ((event.getKeyCode()==KeyEvent.KEYCODE_DEL)&&(event.getAction()==KeyEvent.ACTION_DOWN)) {
						return super.deleteSurroundingText(1,0);
					}
		            if(event.getAction() == KeyEvent.ACTION_DOWN 
		                     && event.getKeyCode() >= KeyEvent.KEYCODE_0 
		                     && event.getKeyCode() <= KeyEvent.KEYCODE_9) {
		                char c = event.getKeyCharacterMap().getNumber(event.getKeyCode());
		                commitText(String.valueOf(c), 1);
		            }
		        }
				return super.sendKeyEvent(event);
			}
			public ExtractedText getExtractedText(ExtractedTextRequest request, int flags) {
				ExtractedText et=new ExtractedText();
				et.text=getEditable().toString();
				//Log.v("EBE","GET "+et.text);
				et.startOffset=0; 
				et.partialStartOffset=-1;
				et.partialEndOffset=-1;
				et.selectionStart=(tisSelStart>=0)?tisSelStart-et.startOffset:-1;
				et.selectionEnd=(tisSelEnd>=0)?tisSelEnd-et.startOffset:-1;
				et.flags=0;
				//et.hint=Hint TODO ?
				tisToken=-1;
				if ((flags&1)!=0)
					tisToken=request.token;
				return et;
			}
		};
		tisEditable=null;
		Editable e=b.getEditable();
		e.clear();
		b.commitText(tisBuffer,tisBuffer.length());
		try {
		b.setSelection(tisSelStart,tisSelEnd);
		} catch (Exception exc) {}
		tisEditable=e;
		tisToken=-1;
		return b;
	}	 
	
	static public boolean setTextInput(final int type,final String buffer,final int selStart,final int selEnd,final String label,final String actionLabel,final String hintText,final String context)
	{
		final Activity activity = WeakActivityHolder.get();
		activity.runOnUiThread(new Runnable() {
		    public void run() {		    	
				tisType=type;
				tisBuffer=buffer;
				tisSelStart=selStart;
				tisSelEnd=selEnd;
				tisLabel=label;
				tisActionLabel=actionLabel;
				tisHint=hintText;
				tisInitCapsMode=((type&0x0F)==1)?(type&0x7000):0;
				tisContext=context;
				int bl=tisBuffer.length();
				Log.v("EBE","STI:"+tisType+" SS:"+tisSelStart+" SE:"+tisSelEnd+" BL:"+bl+" BB:"+buffer+" TE:"+tisEditable);
				if (tisSelStart>bl) tisSelStart=bl;
				if (tisSelEnd>bl) tisSelEnd=bl;
				
				if (tisEditable!=null) {
					tisEditable.replace(0,tisEditable.length(),tisBuffer);
					if ((tisSelStart>=0)&&(tisSelEnd>=0))
						Selection.setSelection(tisEditable,tisSelStart,tisSelEnd);
					else
						Selection.removeSelection(tisEditable);
			    	InputMethodManager imm = (InputMethodManager)
			    			activity.getSystemService(Context.INPUT_METHOD_SERVICE);
						Log.v("EBE","RSI");
						tisToken=-1;
				    	imm.restartInput(mGLView_);
				}
		    }
		});
		return true;
	}
	
	static public String getProperty(String what,String arg) {
		if (what.equals("batteryLevel")) {
			IntentFilter ifilter = new IntentFilter(Intent.ACTION_BATTERY_CHANGED);
			final Activity activity = WeakActivityHolder.get();
			Intent batteryStatus = activity.registerReceiver(null, ifilter);
			if (batteryStatus!=null) {
				int level = batteryStatus.getIntExtra(BatteryManager.EXTRA_LEVEL, -1);
				int scale = batteryStatus.getIntExtra(BatteryManager.EXTRA_SCALE, -1);
				float batteryPct = (float)level * 100 / (float)scale;
				return ""+batteryPct;
			}			
		}

		return "";
	}

	public static void setWindowFlag(Activity activity, final int bits, boolean on) {
		Window win = activity.getWindow();
		WindowManager.LayoutParams winParams = win.getAttributes();
		if (on) {
			winParams.flags |= bits;
		} else {
			winParams.flags &= ~bits;
		}
		win.setAttributes(winParams);
	}

	static public void setProperty(String what,String arg) {
		if (what.equals("statusBar")) {
			if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.R) {
				final Activity activity = WeakActivityHolder.get();
				final String stype=arg;
			    activity.runOnUiThread(new Runnable() {
			    	public void run() {
						WindowInsetsController windowInsetsController = activity.getWindow().getDecorView().getWindowInsetsController();
						if (stype.length()>0) {
							windowInsetsController.show(android.view.WindowInsets.Type.statusBars());
							int ap=0;
							if ("dark".equals(stype))
								ap=WindowInsetsController.APPEARANCE_LIGHT_STATUS_BARS;
							windowInsetsController.setSystemBarsAppearance(ap, WindowInsetsController.APPEARANCE_LIGHT_STATUS_BARS);

							activity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS);
							activity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS);
							//setWindowFlag(activity, WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS, false);
							activity.getWindow().setStatusBarColor(Color.TRANSPARENT);
						}
						else
							windowInsetsController.hide(android.view.WindowInsets.Type.statusBars());
			    	}
			    });
			}
		}
	}

	public boolean onKeyMultiple(int keyCode, int repeatCount, KeyEvent event) {
		if (keyCode==KeyEvent.KEYCODE_UNKNOWN)
			nativeKeyChar(event.getCharacters());
		else if (event.getUnicodeChar()>0)
		{
			String cs=Character.toString((char)event.getUnicodeChar());
			for (int k=0;k<event.getRepeatCount();k++)
				nativeKeyChar(cs);
		}
				
		return false; //XXX what should be return ?
	}

	public boolean isAccelerometerAvailable()
	{
		return accelerometer_.isAvailable();
	}
	
	public void startAccelerometer()
	{
		isAccelerometerStarted_ = true;
		if (isForeground_)
			accelerometer_.enable();
	}

	public void stopAccelerometer()
	{
		isAccelerometerStarted_ = false;
		accelerometer_.disable();
	}


	public boolean isGyroscopeAvailable()
	{
		return gyroscope_.isAvailable();
	}
	
	public void startGyroscope()
	{
		isGyroscopeStarted_ = true;
		if (isForeground_)
			gyroscope_.enable();
	}
	
	public void stopGyroscope()
	{
		isGyroscopeStarted_ = false;
		gyroscope_.disable();
	}
	

	public boolean isGeolocationAvailable()
	{
		return geolocation_.isAvailable();
	}
	
	public boolean isHeadingAvailable()
	{
		return geolocation_.isHeadingAvailable();
	}
	
	public void setGeolocationAccuracy(double accuracy)
	{
		geolocation_.setAccuracy(accuracy);
	}
	
	public double getGeolocationAccuracy()
	{
		return geolocation_.getAccuracy();
	}
	
	public void setGeolocationThreshold(double threshold)
	{			
		geolocation_.setThreshold(threshold);
	}
	
	public double getGeolocationThreshold()
	{
		return geolocation_.getThreshold();
	}
	
	public void startUpdatingLocation()
	{
		isLocationStarted_ = true;
		if (isForeground_)
			geolocation_.startUpdatingLocation();
	}
	
	public void stopUpdatingLocation()
	{
		isLocationStarted_ = false;
		geolocation_.stopUpdatingLocation();
	}
	
	public void startUpdatingHeading()
	{
		isHeadingStarted_ = true;
		if (isForeground_)
			geolocation_.startUpdatingHeading();
	}

	public void stopUpdatingHeading()
	{
		isHeadingStarted_ = false;
		geolocation_.stopUpdatingHeading();		
	}	


	// static equivalents	
	static public boolean checkPermission(String perm) {
		return WeakActivityHolder.get().checkSelfPermission(perm) == PackageManager.PERMISSION_GRANTED;
	}
	static public void requestPermissions(String[] perms) {
		WeakActivityHolder.get().requestPermissions(perms,0);
	}
	static public boolean isAccelerometerAvailable_s()
	{
		return instance_.isAccelerometerAvailable();
	}
	static public void startAccelerometer_s()
	{
		instance_.startAccelerometer();
	}
	static public void stopAccelerometer_s()
	{
		instance_.stopAccelerometer();
	}


	static public boolean isGyroscopeAvailable_s()
	{
		return instance_.isGyroscopeAvailable();
	}
	static public void startGyroscope_s()
	{
		instance_.startGyroscope();
	}
	static public void stopGyroscope_s()
	{
		instance_.stopGyroscope();
	}	

	
	static public boolean isGeolocationAvailable_s()
	{
		return instance_.isGeolocationAvailable();
	}
	static public boolean isHeadingAvailable_s()
	{
		return instance_.isHeadingAvailable();
	}
	static public void setGeolocationAccuracy_s(double accuracy)
	{
		instance_.setGeolocationAccuracy(accuracy);
	}
	static public double getGeolocationAccuracy_s()
	{
		return instance_.getGeolocationAccuracy();
	}
	static public void setGeolocationThreshold_s(double threshold)
	{			
		instance_.setGeolocationThreshold(threshold);
	}
	static public double getGeolocationThreshold_s()
	{
		return instance_.getGeolocationThreshold();
	}
	static public void startUpdatingLocation_s()
	{
		instance_.startUpdatingLocation();
	}
	static public void stopUpdatingLocation_s()
	{
		instance_.stopUpdatingLocation();
	}
	static public void startUpdatingHeading_s()
	{
		instance_.startUpdatingHeading();
	}
	static public void stopUpdatingHeading_s()
	{
		instance_.stopUpdatingHeading();		
	}	

	static public long BackgroundMusicCreateFromFile(String fileName, int[] error)
	{
		return instance_.mediaPlayerManager_.BackgroundMusicCreateFromFile(fileName, error);
	}

	static public void BackgroundMusicDelete(long backgroundMusic)
	{
		instance_.mediaPlayerManager_.BackgroundMusicDelete(backgroundMusic);
	}

	static public int BackgroundMusicGetLength(long backgroundMusic)
	{
		return instance_.mediaPlayerManager_.BackgroundMusicGetLength(backgroundMusic);
	}

	static public long BackgroundMusicPlay(long backgroundMusic, boolean paused, long data)
	{
		return instance_.mediaPlayerManager_.BackgroundMusicPlay(backgroundMusic, paused, data);
	}
	
	static public void BackgroundChannelStop(long backgroundChannel)
	{
		instance_.mediaPlayerManager_.BackgroundChannelStop(backgroundChannel);
	}
	
	static public void BackgroundChannelSetPosition(long backgroundChannel, int position)
	{
		instance_.mediaPlayerManager_.BackgroundChannelSetPosition(backgroundChannel, position);		
	}

	static public int BackgroundChannelGetPosition(long backgroundChannel)
	{
		return instance_.mediaPlayerManager_.BackgroundChannelGetPosition(backgroundChannel);
	}
	
	static public void BackgroundChannelSetPaused(long backgroundChannel, boolean paused)
	{
		instance_.mediaPlayerManager_.BackgroundChannelSetPaused(backgroundChannel, paused);		
	}

	static public boolean BackgroundChannelIsPaused(long backgroundChannel)
	{
		return instance_.mediaPlayerManager_.BackgroundChannelIsPaused(backgroundChannel);	
	}

	static public boolean BackgroundChannelIsPlaying(long backgroundChannel)
	{
		return instance_.mediaPlayerManager_.BackgroundChannelIsPlaying(backgroundChannel);
	}

	static public void BackgroundChannelSetVolume(long backgroundChannel, float volume)
	{
		instance_.mediaPlayerManager_.BackgroundChannelSetVolume(backgroundChannel, volume);		
	}

	static public float BackgroundChannelGetVolume(long backgroundChannel)
	{
		return instance_.mediaPlayerManager_.BackgroundChannelGetVolume(backgroundChannel);
	}

	static public void BackgroundChannelSetLooping(long backgroundChannel, boolean looping)
	{
		instance_.mediaPlayerManager_.BackgroundChannelSetLooping(backgroundChannel, looping);		
	}

	static public boolean BackgroundChannelIsLooping(long backgroundChannel)
	{
		return instance_.mediaPlayerManager_.BackgroundChannelIsLooping(backgroundChannel);		
	}

	int fps_ = 60;
	static public void setFps(int fps)
	{
		instance_.fps_ = fps;		
	}	
	
	
	static public void setKeepAwake(boolean awake)
	{
		final Activity activity = WeakActivityHolder.get();
		
		if (awake)
			activity.runOnUiThread(new Runnable() {public void run() {activity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);}});
		else
			activity.runOnUiThread(new Runnable() {public void run() {activity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);}});
	}
	
	static public boolean setKeyboardVisibility(final boolean visible)
	{
		final Activity activity = WeakActivityHolder.get();
		activity.runOnUiThread(new Runnable() {
		    public void run() {
		    	activity.getWindow().setSoftInputMode(visible?WindowManager.LayoutParams.SOFT_INPUT_STATE_VISIBLE:WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_HIDDEN);
		    	mGLView_.clearFocus();
		    	if (visible)
		    		mGLView_.requestFocus();
		    	
		    	InputMethodManager imm = (InputMethodManager)
	    			activity.getSystemService(Context.INPUT_METHOD_SERVICE);
		    	if (visible)
		    		imm.showSoftInput(mGLView_ , 0/*InputMethodManager.SHOW_FORCED*/);
		    	else
		    	{
		    		imm.hideSoftInputFromWindow(mGLView_.getWindowToken() ,0); 
		    		activity.onWindowFocusChanged(activity.hasWindowFocus());
		    	}
		    }
		});
		return true;
	}

	static public void vibrate(int ms)	
	{
		try
		{
			((Vibrator)WeakActivityHolder.get().getSystemService(Context.VIBRATOR_SERVICE)).vibrate(ms);
		}
		catch (SecurityException e)
		{
		}
	}	

	static public void enableOnDemand_s(boolean en)
	{
		onDemandEnabled=en;
		instance_.enableOnDemand(en);
	}

	static public void requestDraw_s()
	{
		instance_.requestDraw();
	}

	static public String getLocale()
	{
		Locale locale = Locale.getDefault();
		return locale.getLanguage() + "_" + locale.getCountry();
	}

	static public String getLanguage()
	{
		Locale locale = Locale.getDefault();
		return locale.getLanguage();
	}
	
	static public String getAppId()
	{
		final Activity activity = WeakActivityHolder.get();
		return activity.getPackageName();
	}

	static public String getVersion()
	{
		return android.os.Build.VERSION.RELEASE;
	}

	static public String getManufacturer()
	{
		return android.os.Build.MANUFACTURER;
	}

	static public String getModel()
	{
		return android.os.Build.MODEL;
	}
    
    static public String getDeviceType()
	{
    	//Recommanded way of detecting a TV device
    	if (WeakActivityHolder.get().getPackageManager().hasSystemFeature(PackageManager.FEATURE_LEANBACK))
            return "TV";
    	//Generic way
    	UiModeManager uiModeManager = (UiModeManager) WeakActivityHolder.get().getSystemService(Context.UI_MODE_SERVICE);
        if (uiModeManager.getCurrentModeType() == Configuration.UI_MODE_TYPE_TELEVISION) {
            return "TV";
        } else if (uiModeManager.getCurrentModeType() == Configuration.UI_MODE_TYPE_APPLIANCE) {
            return "Appliance";
        } else if (uiModeManager.getCurrentModeType() == Configuration.UI_MODE_TYPE_CAR) {
            return "Car";
        } else if (uiModeManager.getCurrentModeType() == Configuration.UI_MODE_TYPE_DESK) {
            return "Desk";
        } else if (uiModeManager.getCurrentModeType() == Configuration.UI_MODE_TYPE_WATCH) {
            return "Watch";
        } else {
            return "Mobile";
        }
	}

    static public boolean setClipboard(String data, String mime) {
		final Activity activity = WeakActivityHolder.get();
    	ClipboardManager clipboard = (ClipboardManager)
    	        activity.getSystemService(Context.CLIPBOARD_SERVICE);
    	if (mime.startsWith("text/")) {
    		ClipData clip = ClipData.newPlainText(mime, data);
    		clipboard.setPrimaryClip(clip);
    		return true;
    	}
    	return false;
    }

    static public String[] getClipboard(String mime) {
		final Activity activity = WeakActivityHolder.get();
    	ClipboardManager clipboard = (ClipboardManager)
    	        activity.getSystemService(Context.CLIPBOARD_SERVICE);
    	if (clipboard.hasPrimaryClip()) {
    	    ClipData clip = clipboard.getPrimaryClip();
    	    if (clip.getDescription().hasMimeType(mime)) {
    	    	if (mime.startsWith("text/")) {
    	    		return new String[] {
    	    				clip.getItemAt(0).coerceToText(activity).toString(), mime
    	    		};
    	    	}
    	    }
    	}
    	return null;
    }
    
	static public void openUrl(String url)
	{
		Uri uri=Uri.parse(url);
		String scheme=uri.getScheme();
		Intent intent=null;
		if (scheme!=null)
		{
			if (scheme.equals("LAUNCH"))
			{
				PackageManager manager = WeakActivityHolder.get().getPackageManager();
		        Intent i = manager.getLaunchIntentForPackage(uri.getSchemeSpecificPart());
		        if (i != null) {
			        i.addCategory(Intent.CATEGORY_LAUNCHER);
			        intent=i;
		        }
			}
			else if (scheme.equals("INTENT"))
				intent=new Intent(uri.getSchemeSpecificPart());
		}
		if (intent==null)
			intent = new Intent(Intent.ACTION_VIEW, uri);

		try
		{
			WeakActivityHolder.get().startActivity(intent);
		}
		catch (ActivityNotFoundException e) 
		{
		}
	}

	static public boolean canOpenUrl(String url)
	{
		try
		{
			Activity activity = WeakActivityHolder.get();
			PackageManager pm = activity.getPackageManager();
			Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
			ComponentName cn = intent.resolveActivity(pm);

			if (cn != null)
				return true;
		}
		catch (Exception e)
		{
		}

		return false;
	}
	
	static public void finishActivity()
	{
		final Activity activity = WeakActivityHolder.get();

		activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
            	activity.finish();
            }
        });
	}
	
	static public int getScreenDensity()
	{
		DisplayMetrics dm = new DisplayMetrics();
		WeakActivityHolder.get().getWindowManager().getDefaultDisplay().getMetrics(dm);
		return dm.densityDpi;		
	}

	public static String getLocalIPs()
	{
		StringBuilder sb = new StringBuilder();

		try
		{
			for (Enumeration<NetworkInterface> en = NetworkInterface
					.getNetworkInterfaces(); en.hasMoreElements();)
			{
				NetworkInterface intf = en.nextElement();
				for (Enumeration<InetAddress> enumIpAddr = intf
						.getInetAddresses(); enumIpAddr.hasMoreElements();)
				{
					InetAddress inetAddress = enumIpAddr.nextElement();
					if (!inetAddress.isLoopbackAddress())
					{
						sb.append(inetAddress.getHostAddress().toString());
						sb.append("|");
					}
				}
			}
		} catch (SocketException ex)
		{
		}

		if (sb.length() != 0)
			sb.deleteCharAt(sb.length() - 1);

		return sb.toString();
	}	
	
	public static int getRotation(int w,int h)
	{
		Activity activity = WeakActivityHolder.get();

		int rotation = activity.getWindowManager().getDefaultDisplay().getRotation();
		
		if (w<=h)
		{
			if (rotation == Surface.ROTATION_0 || rotation == Surface.ROTATION_270)
				return 0;
			else
				return 180;
		}
		else
		{
			if (rotation == Surface.ROTATION_0 || rotation == Surface.ROTATION_90)
				return 90;
			else
				return 270;
		}
	}
	
	static public String getDeviceName() {
		String manufacturer = android.os.Build.MANUFACTURER;
		String model = android.os.Build.MODEL;
		if (model.startsWith(manufacturer)) {
			return model;
		} else {
			return manufacturer + " " + model;
		}
	}
	
	static String toJFunctionName(String fn)
	{
		return fn.trim().replaceAll("\\s","_").replaceAll("\\W","");
	}
	
	static String toJFileName(String fn)
	{
		return fn.trim().replaceAll("\\s","_").replaceAll("[^a-zA-Z0-9_\\-\\.]","");
	}
	
	static public void throwLuaException(String error) throws Exception{
		if ((error!=null)&&error.contains("stack traceback:"))
		{
			int sidx=error.indexOf("stack traceback:\n\t");
			String[] stack=error.substring(sidx+18).split("\n\t");
			LuaException le=null;
			try {
				StackTraceElement[] st = new StackTraceElement[stack.length];
				for (int k=0;k<stack.length;k++)
				{
					String te=stack[k];
					int sepsearch=0;
					if (te.startsWith("["))
						sepsearch=te.indexOf(']')+1;
					int sep1=te.indexOf(' ',sepsearch);
					if (sep1>=0)
					{
						String[] fl=te.substring(0,sep1).split(":");
						String fn=te.substring(sep1+1);
						int ln=1234;
						if (fl.length>1)
						{
							try {
								ln=Integer.parseInt(fl[1]);
							}
							catch (Exception pe)
							{								
								throw pe;
							}
						}
						if (fn.startsWith("in function "))
							fn=fn.substring(12);
						else if (fn.startsWith("in main chunk"))
							fn="MAIN_CHUNK";
						st[k]=new StackTraceElement("LUA",toJFunctionName(fn),toJFileName(fl[0]),ln);						
					}
					else
						st[k]=new StackTraceElement("LUA",toJFunctionName(te),"",0);
				}
				le=new LuaException(error/*.substring(0,sidx)*/);
				le.setStackTrace(st);
			}
			catch (Exception se)
			{			
				throw se;
			}
			throw le;
		}
		throw new LuaException(error);
	}

	static private native void nativeOpenProject(String project);
	static private native void nativeLowMemory();
	static private native boolean isRunning();
	static private native boolean nativeKeyDown(int keyCode, int repeatCount);
	static private native boolean nativeKeyUp(int keyCode, int repeatCount);
	static private native void nativeKeyChar(String keyChar);
	static private native void nativeTextInput(String buffer,int selStart,int selEnd, String context);
	static private native void nativeOpenALSetup(int sampleRate);
	static private native void nativeCreate(boolean player,Activity activity);
	static private native void nativeSetDirectories(String externalDir, String internalDir, String cacheDir);
	static private native void nativeSetFileSystem(String files);
	static private native void nativePause();
	static private native void nativeResume();
	static private native void nativeDestroy();
	static private native void nativeSurfaceCreated(Surface surface);
	static private native void nativeSurfaceChanged(int w, int h, int rotation,Surface surface);
	static private native void nativeSurfaceDestroyed();
	static private native void nativeDrawFrame(boolean tick);
	static private native void nativeTick();
	static private native void nativeMouseWheel(int x,int y,int button,float amount);
	static private native void nativeTouchesBegin(int size, int[] id, int[] x, int[] y, float[] pressure, int actionIndex);
	static private native void nativeTouchesMove(int size, int[] id, int[] x, int[] y, float[] pressure);
	static private native void nativeTouchesEnd(int size, int[] id, int[] x, int[] y, float[] pressure, int actionIndex);
	static private native void nativeTouchesCancel(int size, int[] id, int[] x, int[] y, float[] pressure);
	static private native void nativeStop();
	static private native void nativeStart();
	static private native void nativeHandleOpenUrl(String url);
	static private native void oculusStop();
	static private native void oculusStart();
	static private native void oculusPause();
	static private native void oculusResume();
	static public native void oculusRunThread();
	static public native void oculusPostCreate();
}
