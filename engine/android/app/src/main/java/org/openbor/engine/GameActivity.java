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

import java.io.InputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.io.File;
import java.io.FileOutputStream;

import android.util.Log;
import android.os.Bundle;
import android.content.Context;
import android.os.Build;
import android.content.pm.PackageManager;
import android.content.pm.ApplicationInfo;
import android.os.PowerManager;
import android.os.PowerManager.*;
import android.view.View;
import android.view.WindowManager;
import android.content.res.*;
import android.Manifest;
//msmalik681 added imports for new pak copy!
import android.os.Environment;
import android.widget.Toast;
//msmalik681 added import for permission check
import android.support.v4.content.ContextCompat;
import android.support.v4.app.ActivityCompat;
import android.os.Vibrator;
import android.os.VibrationEffect;
import android.view.*;

/**
 * Extended functionality from SDLActivity.
 *
 * Separated for ease of updating both for dependency and this support functionality later.
 */
public class GameActivity extends SDLActivity {
  /**
   * Needed for permission check
   */
  public static final int STORAGE_PERMISSION_CODE = 23;
  
  //White Dragon: added statics
  protected static WakeLock wakeLock;
  protected static View decorView;

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

    //msmalik681 setup storage access
    CheckPermissionForMovingPaks();

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

  //msmalik681 added permission check for API 23+ for moving .paks
  private void CheckPermissionForMovingPaks() {
    if (Build.VERSION.SDK_INT >= STORAGE_PERMISSION_CODE &&
        getApplicationContext().getPackageName().equals("org.openbor.engine"))
    {
      if (ContextCompat.checkSelfPermission(GameActivity.this, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED &&
          ContextCompat.checkSelfPermission(GameActivity.this, Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED)
      {
        Toast.makeText(this, "Needed permissions not granted!", Toast.LENGTH_LONG).show();
        ActivityCompat.requestPermissions(GameActivity.this, new String[] {
          Manifest.permission.WRITE_EXTERNAL_STORAGE,
          Manifest.permission.READ_EXTERNAL_STORAGE
        }, STORAGE_PERMISSION_CODE);
      }
      else 
      {
        CopyPak();
      }
    }
    else
    {
      CopyPak();
    }
  }

  @Override
  public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults)
  {
    switch (requestCode)
    {
      case STORAGE_PERMISSION_CODE:
      {
        // If request is cancelled, the result arrays are empty.
        if (grantResults.length > 0 &&
            grantResults[0] == PackageManager.PERMISSION_GRANTED)
        {
          // permission was granted continue!
          CopyPak();
        }          
        else
        {
          // needed permission denied end application!
          finish();
        }
      }
    }
  }

  /**
   * Proceed in copying paks files, or just prepare the destination Paks directory depending
   * on which type of app it is.
   */
  public void CopyPak()
  {
    try {
      Context ctx = getContext();
      Context appCtx = getApplicationContext();
      String toast = null;

      // if package name is literally "org.openbor.engine" then we have no need to copy any .pak files
      if (appCtx.getPackageName().equals("org.openbor.engine"))
      {
        // Default output folder
        File outFolderDefault = new File(Environment.getExternalStorageDirectory() + "/OpenBOR/Paks");

        if (!outFolderDefault.isDirectory())
        {
          outFolderDefault.mkdirs();
          toast = "Folder: (" + outFolderDefault + ") is empty!";
          Toast.makeText(appCtx, toast, Toast.LENGTH_LONG).show();
        }
        else
        {
          String[] files = outFolderDefault.list();
          if (files.length == 0)
          {
            // directory is empty
            toast = "Paks Folder: (" + outFolderDefault + ") is empty!";
            Toast.makeText(appCtx, toast, Toast.LENGTH_LONG).show();
          }
        }
      }
      // otherwise it acts like a dedicated app (commercial title, standalone app)
      // intend to work with pre-baked single .pak file at build time
      else
      {
        String version = null;
        // versionName is "android:versionName" in AndroidManifest.xml
        version = appCtx.getPackageManager().getPackageInfo(appCtx.getPackageName(), 0).versionName;  // get version number as string
        // set local output folder (primary shared/external storage)
        File outFolder = new File(ctx.getExternalFilesDir(null) + "/Paks");
        // set local output filename as version number
        File outFile = new File(outFolder, version + ".pak");

        // check if existing pak directory is actually directory, and pak file with matching version
        // for this build is there, if not then delete all files residing in such
        // directory (old pak files) preparing for updating new one
        if (outFolder.isDirectory() && !outFile.exists()) // if local folder true and file does not match version empty folder
        {
          toast = "Updating please wait!";
          String[] children = outFolder.list();
          for (int i = 0; i < children.length; i++)
          {
            new File(outFolder, children[i]).delete();
          }
        }
        else
        {
          toast = "First time setup, please wait...";
        }

        if (!outFile.exists())
        {
          Toast.makeText(appCtx, toast, Toast.LENGTH_LONG).show();
          outFolder.mkdirs();

		  //custom pak should be saved in "app\src\main\assets\bor.pak"
		  InputStream in = ctx.getAssets().open("bor.pak");
          FileOutputStream out = new FileOutputStream(outFile);

          copyFile(in, out);
          in.close();
          in = null;
          out.flush();
          out.close();
          out = null;
        }
      }
    } catch (IOException e) {
      // not handled
    } catch (Exception e) {
      // not handled
    }
  }

  private void copyFile(InputStream in, OutputStream out) throws IOException {
    byte[] buffer = new byte[1024];
    int read;
    while ((read = in.read(buffer)) != -1)
    {
      out.write(buffer, 0, read);
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
