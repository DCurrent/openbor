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
import android.Manifest;
import android.app.AppOpsManager;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.provider.Settings;
import android.widget.Toast;
import java.io.File;
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

  // ------------------------------------------------------------------------ //
  // Frontend launch.
  //
  // A frontend (Daijishō, Pegasus, ES-DE, a launcher script...) can start this
  // activity directly and name the pak to run, so the player lands in the game
  // instead of the pak selection menu:
  //
  //   am start -n org.openbor.engine/.GameActivity \
  //            -e pak /storage/emulated/0/Games/openbor/Game.pak
  //
  // The path travels to the native main() as its single command line
  // argument, the same way every desktop port takes a pak on the command
  // line. Without the extra, or when the file cannot be read, nothing changes:
  // the engine shows its menu as before.
  //
  // A pak outside this app's own storage needs a shared storage permission.
  // When it is missing, the user gets the permission dialog (Android 10 and
  // older) or the "All files access" settings page (Android 11 and later)
  // and can retry from the frontend afterwards.
  // ------------------------------------------------------------------------ //

  /** Intent string extra that carries the absolute path of the pak to launch. */
  public static final String EXTRA_PAK_PATH = "pak";

  /** Pak path taken from the launching intent, or null for the menu flow. */
  private String frontendPakPath = null;

  /**
   * Hands the pak named by the launching intent to the native main() as
   * argv[1]. SDL calls this on its own thread after onCreate() has run.
   */
  @Override
  protected String[] getArguments()
  {
    if (frontendPakPath == null)
    {
      return super.getArguments();
    }

    return new String[] { frontendPakPath };
  }

  /**
   * Reads EXTRA_PAK_PATH from the launching intent and checks that the engine
   * will be able to open the file.
   *
   * @return the pak path to run, or null to fall back to the menu.
   */
  private String resolveFrontendPak()
  {
    Intent intent = getIntent();

    if (intent == null)
    {
      return null;
    }

    String path = intent.getStringExtra(EXTRA_PAK_PATH);

    if (path == null || path.isEmpty())
    {
      return null;
    }

    File pak = new File(path);

    if (pak.canRead())
    {
      Log.i("OpenBOR", "Launching pak from intent: " + path);
      return path;
    }

    // Without a shared storage permission the system hides the file, so
    // exists() cannot tell a missing pak from a blocked one. Ask for the
    // permission first; the frontend can launch again once it is granted.
    if (!hasSharedStoragePermission())
    {
      Log.w("OpenBOR", "Pak from intent needs shared storage permission: " + path);
      Toast.makeText(this, "OpenBOR needs storage access to open paks from other apps. Grant it, then launch again.", Toast.LENGTH_LONG).show();
      requestSharedStoragePermission();
      return null;
    }

    Log.w("OpenBOR", "Pak from intent not found or unreadable: " + path);
    Toast.makeText(this, "Pak not found: " + path, Toast.LENGTH_LONG).show();
    return null;
  }

  /** Request code for the classic storage permission dialog (Android 10 and older). */
  private static final int REQUEST_SHARED_STORAGE = 0x504B; // "PK"

  /**
   * App op behind the "All files access" switch (AppOpsManager.OPSTR_MANAGE_EXTERNAL_STORAGE,
   * which the SDK does not expose).
   */
  private static final String OP_MANAGE_EXTERNAL_STORAGE = "android:manage_external_storage";

  /**
   * @return true when this app may read files outside its own storage:
   *         "All files access" on Android 11 and later, the classic storage
   *         permission before that.
   */
  private boolean hasSharedStoragePermission()
  {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M)
    {
      // Install-time permission model: granted with the manifest entry.
      return true;
    }

    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R)
    {
      return checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED;
    }

    // Same rule the platform applies for Environment.isExternalStorageManager():
    // the op is on, or it is untouched and the permission itself was granted.
    // Evaluated here because some vendor builds return true from that method
    // while the settings switch is still off and every read fails.
    AppOpsManager ops = getSystemService(AppOpsManager.class);
    int mode = ops.unsafeCheckOpNoThrow(OP_MANAGE_EXTERNAL_STORAGE, getApplicationInfo().uid, getPackageName());

    if (mode == AppOpsManager.MODE_ALLOWED)
    {
      return true;
    }

    return mode == AppOpsManager.MODE_DEFAULT
        && checkSelfPermission(Manifest.permission.MANAGE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED;
  }

  /**
   * Asks for the shared storage permission. Android 11 and later only grant
   * "All files access" from a system settings page, so that page is opened
   * for this app; older versions show the normal permission dialog.
   */
  private void requestSharedStoragePermission()
  {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M)
    {
      return;
    }

    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R)
    {
      requestPermissions(new String[] { Manifest.permission.READ_EXTERNAL_STORAGE }, REQUEST_SHARED_STORAGE);
      return;
    }

    Intent settings = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION, Uri.parse("package:" + getPackageName()));

    try
    {
      startActivity(settings);
    }
    catch (android.content.ActivityNotFoundException e)
    {
      Log.w("OpenBOR", "No settings page for All files access: " + e.getMessage());
    }
  }
  // ------------------------------------------------------------------------ //

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

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    // call parent's implementation
    super.onCreate(savedInstanceState);
    Log.v("OpenBOR", "onCreate called");

    // Frontend launch: pick up the pak named in the intent, if any.
    frontendPakPath = resolveFrontendPak();
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
