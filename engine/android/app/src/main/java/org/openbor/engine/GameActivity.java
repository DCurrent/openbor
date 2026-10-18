/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) OpenBOR Team
 *
 * Moved from SDLActivity.java here for more flexibility.
 * IMPORTANT: DON'T EDIT SDLActivity.java anymore, but this file!
 *
 * The following from SDLActivity.java migration, and kept intact for respect to authors
 * as well as specific lines inside this source file is kept intact although moved / rearranged /
 * removed / modified as part from migration process.
 * --------------------------------------------------------
 * SDLActivity.java - Main code for Android build.
 * Original by UTunnels (utunnels@hotmail.com).
 * Modifications by CRxTRDude, White Dragon and msmalik681.
 * --------------------------------------------------------
 */

package org.openbor.engine;

import org.libsdl.app.SDLActivity;

import java.io.File;

import android.util.Log;
import android.os.Bundle;
import android.content.Context;
import android.os.Build;
import android.content.pm.ApplicationInfo;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock; // Explicitly import WakeLock if you want to be specific, or keep PowerManager.*
import android.view.View;
import android.view.WindowManager;
import android.view.WindowManager;
import android.os.Vibrator;
import android.os.VibrationEffect;

import android.content.BroadcastReceiver;
import android.content.Intent;
import android.content.IntentFilter;
import org.jetbrains.annotations.Nullable;

/**
 * Extended functionality from SDLActivity.
 *
 * Separated for ease of updating both for dependency and this support functionality later.
 */
public class GameActivity extends SDLActivity {

  //White Dragon: added statics
  protected static WakeLock wakeLock;
  protected static View decorView;

//needed to fix sdk 34+ crashing
@Override
public Intent registerReceiver(@Nullable BroadcastReceiver receiver, IntentFilter filter) {
    if (Build.VERSION.SDK_INT >= 34 && getApplicationInfo().targetSdkVersion >= 34) {
        return super.registerReceiver(receiver, filter, Context.RECEIVER_EXPORTED);
    } else {
        return super.registerReceiver(receiver, filter);
    }
}
  //note: White Dragon's vibrator is moved into C code for 2 reasons
  // - avoid modifying SDLActivity.java as it's platform support
  // - reduce round-trip cost/time in call C-function to check for touch-area and whether
  //   vibration is enabled or not
  //   (for reference: SDL finally registers event/action/x/y/etc into its C-code from Java code
  //   in onTouch() call, thus we do this logic in C code for efficient then provide vibration code
  //   in Java when we really need to vibrate the device)
  
  // -- section of Java native solutions provided to be called from C code -- //
  /**
   * This will vibrate device if there's vibrator service.
   * Otherwise it will do nothing.
   *
   * Modified version from original by White Dragon
   */
  public static void jni_vibrate() {
    Vibrator vibrator = (Vibrator)getContext().getSystemService(Context.VIBRATOR_SERVICE);

    if (vibrator.hasVibrator())
    {

      // wait for 3 ms, vibrate for 250 ms, then off for 1000 ms
      // note: consult api at two links below, it has two different meanings but in this case,
      // use case is the same
      long[] pattern = {16, 250};

      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
      {
        // API 26 and above
        // look for its api at https://developer.android.com/reference/android/os/VibrationEffect.html
        vibrator.vibrate(VibrationEffect.createWaveform(pattern, -1));
      }
      else
      {
        // below API 26
        // look for its api at https://developer.android.com/reference/android/os/Vibrator.html#vibrate(long%5B%5D,%2520int)
        vibrator.vibrate(pattern, -1);
      }
    }
  } 
  // ------------------------------------------------------------------------ //

  /**
   * Also load "openbor" as shared library to run the game in which
   * inside there's main function entry for the program.
   */
  @Override
  protected String[] getLibraries() {
    return new String[] {
      "SDL2",
      "openbor"
    };
  }

  /**
   * Returns the library holding SDL_main ("libopenbor.so").
   *
   * SDL 2.0.10 dlopen()s this exact string. The stock version returns an absolute path in
   * nativeLibraryDir, which only exists when the installer extracts the .so files. A
   * Play/App Bundle install leaves them unextracted inside the split APK, so that path is
   * missing and the game closes immediately. When the extracted file is absent we return
   * the bare soname so the linker loads it straight from the split APK instead.
   */
  @Override
  protected String getMainSharedObject() {
    String[] libraries = getLibraries();
    String libraryName = "lib" + libraries[libraries.length - 1] + ".so";
    String extractedPath = getContext().getApplicationInfo().nativeLibraryDir + "/" + libraryName;

    if (new File(extractedPath).exists()) {
      return extractedPath;
    }
    return libraryName;
  }

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    // call parent's implementation
    super.onCreate(savedInstanceState);
    Log.v("OpenBOR", "onCreate called");
    //msmalik681 copy pak for custom apk and notify is paks folder empty
   // CopyPak();

    //CRxTRDude - Added FLAG_KEEP_SCREEN_ON to prevent screen timeout.
    getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

    //CRxTRDude - Created a wakelock to prevent the app from being shut down upon screen lock.
    PowerManager pm = (PowerManager)getSystemService(POWER_SERVICE);
    GameActivity.wakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "BOR");
    if (!GameActivity.wakeLock.isHeld())
    {
      GameActivity.wakeLock.acquire();
    }
  }

  @Override
  public void onLowMemory() {
    super.onLowMemory();
    Log.v("OpenBOR", "onLowMemory");

    //CRxTRDude - Release wakelock first before destroying.
    if (GameActivity.wakeLock.isHeld())
      GameActivity.wakeLock.release();
  }

  @Override
  protected void onPause() {
    super.onPause();
    Log.v("OpenBOR", "onPause");

    //White Dragon: wakelock release!
    if (GameActivity.wakeLock.isHeld())
      GameActivity.wakeLock.release();
  }

  @Override
  protected void onResume() {
    super.onResume();
    Log.v("OpenBOR", "onResume");

    //White Dragon: wakelock acquire!
    if (!GameActivity.wakeLock.isHeld())
      GameActivity.wakeLock.acquire();
  }

  @Override
  protected void onDestroy() {
    super.onDestroy();
    Log.v("OpenBOR", "onDestroy");

    //CRxTRDude - Release wakelock first before destroying.
    if (GameActivity.wakeLock.isHeld())
      GameActivity.wakeLock.release();
  }
}
