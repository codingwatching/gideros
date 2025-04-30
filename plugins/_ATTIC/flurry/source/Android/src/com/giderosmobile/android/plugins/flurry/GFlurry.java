package com.giderosmobile.android.plugins.flurry;

import java.lang.ref.WeakReference;
import java.util.Map;

import android.app.Activity;

import com.flurry.android.FlurryAgent;

public class GFlurry
{
	private static WeakReference<Activity> sActivity;
	private static volatile boolean sIsActive = true;
	private static volatile boolean isInitiated = false;
	private static String sApiKey = null;

	public static void onCreate(Activity activity)
	{
		sActivity = new WeakReference<Activity>(activity);
		FlurryAgent.setLogEnabled(false);
	}

	public static void onStart()
	{
		if (isInitiated && !sIsActive)
		{
			FlurryAgent.onStartSession(sActivity.get());
			sIsActive = true;
		}
	}

	public static void onStop()
	{
		if (isInitiated && sIsActive)
		{
			FlurryAgent.onEndSession(sActivity.get());
			sIsActive = false;
		}
	}

	public static void startSession(String apiKey)
	{
		sApiKey = apiKey;
		if (!isInitiated)
		{
			FlurryAgent.init(sActivity.get(), sApiKey);
            FlurryAgent.onStartSession(sActivity.get());
			isInitiated = true;
            sIsActive = true;
		}
	}

	public static void logEvent(String eventId)
	{
		FlurryAgent.logEvent(eventId);
	}

	public static void logEvent(String eventId, Map<String, String> parameters)
	{
		FlurryAgent.logEvent(eventId, parameters);
	}

	public static void logEvent(String eventId, boolean timed)
	{
		FlurryAgent.logEvent(eventId, timed);
	}

	public static void logEvent(String eventId, Map<String, String> parameters, boolean timed)
	{
		FlurryAgent.logEvent(eventId, parameters, timed);
	}

	public static void endTimedEvent(String eventId)
	{
		FlurryAgent.endTimedEvent(eventId);
	}

	public static void endTimedEvent(String eventId, Map<String, String> parameters)
	{
		FlurryAgent.endTimedEvent(eventId, parameters);
	}
}
