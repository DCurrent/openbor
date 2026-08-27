package org.openbor.engine;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.res.AssetFileDescriptor;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.Locale;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/*
- Caskey, Damon V.
- 2026-08-24
-
- Prepare the game PAK bundled with a standalone Android package before
- starting OpenBOR. Each packaged game owns its private application storage.
*/
public class LauncherActivity extends Activity {

    private static final String TAG = "LauncherActivity";
    private static final int COPY_BUFFER_SIZE = 64 * 1024;
    private static final String PAK_DIRECTORY_NAME = "Paks";
    private static final String BUNDLED_PAK_NAME = "bor.pak";
    private static final String PAK_VERSION_MARKER_NAME = ".installed-pak-version";

    private final ExecutorService installExecutor = Executors.newSingleThreadExecutor();
    private String applicationName = "OpenBOR";
    private String installedPakName = "game.pak";
    private File pakDirectory;
    private boolean gameStarted;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        applicationName = getApplicationName();
        installedPakName = getString(R.string.installed_pak_name);

        File externalFilesDirectory = getExternalFilesDir(null);
        if (externalFilesDirectory == null) {
            showStorageError("Application storage is unavailable.");
            return;
        }

        pakDirectory = new File(externalFilesDirectory, PAK_DIRECTORY_NAME);
        if (!pakDirectory.isDirectory() && !pakDirectory.mkdirs()) {
            showStorageError("The game data folder could not be created.");
            return;
        }

        preparePackagedGame();
    }

    /*
    - Caskey, Damon V.
    - 2026-08-24
    -
    - Install bundled game data on a worker thread. OpenBOR starts only after
    - the copied PAK is flushed, closed, validated, and published.
    */
    private void preparePackagedGame() {
        installExecutor.execute(() -> {
            try {
                if (!hasBundledPak()) {
                    runOnUiThread(this::showPackagingError);
                    return;
                }

                installBundledPak();
                runOnUiThread(this::startGameActivity);
            } catch (Exception exception) {
                Log.e(TAG, "Could not prepare bundled game data.", exception);
                runOnUiThread(() -> showStorageError(safeMessage(exception)));
            }
        });
    }

    /*
    - Caskey, Damon V.
    - 2026-08-24
    -
    - Copy to a temporary file beside the destination, synchronize it to disk,
    - then replace the installed PAK without exposing a partial file.
    */
    private void installBundledPak() throws Exception {
        long versionCode = getVersionCode();
        File destinationFile = new File(pakDirectory, installedPakName);
        File versionMarker = new File(pakDirectory, PAK_VERSION_MARKER_NAME);
        long expectedSize = getBundledPakSize();

        if (isCompleteFile(destinationFile, expectedSize)
            && isInstalledVersion(versionMarker, versionCode)) {
            removeOldPaks(destinationFile);
            return;
        }

        File temporaryFile = new File(pakDirectory, destinationFile.getName() + ".part");
        deleteQuietly(temporaryFile);

        try (InputStream input = getAssets().open(BUNDLED_PAK_NAME)) {
            copyToTemporaryFile(input, temporaryFile, expectedSize);
            replaceFile(temporaryFile, destinationFile);
            writeVersionMarker(versionMarker, versionCode);
        } catch (Exception exception) {
            deleteQuietly(temporaryFile);
            throw exception;
        }

        removeOldPaks(destinationFile);
        Log.i(TAG, "Installed bundled PAK: " + destinationFile.getAbsolutePath());
    }

    private void copyToTemporaryFile(
        InputStream input,
        File temporaryFile,
        long expectedSize) throws IOException {

        long copiedSize = 0;
        byte[] buffer = new byte[COPY_BUFFER_SIZE];

        try (FileOutputStream output = new FileOutputStream(temporaryFile, false)) {
            int bytesRead;
            while ((bytesRead = input.read(buffer)) != -1) {
                output.write(buffer, 0, bytesRead);
                copiedSize += bytesRead;
            }
            output.flush();
            output.getFD().sync();
        }

        if (copiedSize <= 0 || temporaryFile.length() != copiedSize) {
            throw new IOException("The bundled PAK copy is incomplete.");
        }
        if (expectedSize >= 0 && copiedSize != expectedSize) {
            throw new IOException(
                "The bundled PAK size is " + copiedSize + " bytes; expected " + expectedSize + ".");
        }
    }

    /*
    - Caskey, Damon V.
    - 2026-08-24
    -
    - Preserve the previous installed PAK until the completed replacement is
    - in place. Both renames stay inside one directory for atomic publication.
    */
    private void replaceFile(File temporaryFile, File destinationFile) throws IOException {
        File backupFile = new File(pakDirectory, destinationFile.getName() + ".bak");
        deleteQuietly(backupFile);

        boolean hadDestination = destinationFile.isFile();
        if (hadDestination && !destinationFile.renameTo(backupFile)) {
            throw new IOException("The previous game data could not be preserved.");
        }

        if (!temporaryFile.renameTo(destinationFile)) {
            if (hadDestination && !backupFile.renameTo(destinationFile)) {
                Log.e(TAG, "Could not restore the previous PAK after replacement failed.");
            }
            throw new IOException("The completed game data could not be installed.");
        }

        deleteQuietly(backupFile);
    }

    private boolean hasBundledPak() throws IOException {
        String[] assetNames = getAssets().list("");
        if (assetNames == null) {
            return false;
        }

        for (String assetName : assetNames) {
            if (BUNDLED_PAK_NAME.equals(assetName)) {
                return true;
            }
        }
        return false;
    }

    private long getBundledPakSize() {
        try (AssetFileDescriptor descriptor = getAssets().openFd(BUNDLED_PAK_NAME)) {
            return descriptor.getLength();
        } catch (IOException exception) {
            Log.i(TAG, "Bundled PAK length is unavailable; copy validation will use bytes written.");
            return -1;
        }
    }

    private long getVersionCode() throws Exception {
        PackageInfo packageInfo = getPackageManager().getPackageInfo(getPackageName(), 0);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            return packageInfo.getLongVersionCode();
        }
        return packageInfo.versionCode;
    }

    private boolean isCompleteFile(File file, long expectedSize) {
        if (!file.isFile() || file.length() <= 0) {
            return false;
        }
        return expectedSize < 0 || file.length() == expectedSize;
    }

    /*
     * Caskey, Damon V.
     * 2026-08-27
     *
     * Preserve a creator-selected PAK name across application updates while
     * using Android's monotonically increasing version code to refresh data.
     */
    private boolean isInstalledVersion(File versionMarker, long versionCode) {
        if (!versionMarker.isFile()) {
            return false;
        }

        byte[] value = new byte[64];
        try (InputStream input = new FileInputStream(versionMarker)) {
            int bytesRead = input.read(value);
            if (bytesRead <= 0) {
                return false;
            }
            String installedVersion = new String(
                value, 0, bytesRead, StandardCharsets.US_ASCII).trim();
            return Long.toString(versionCode).equals(installedVersion);
        } catch (IOException exception) {
            Log.w(TAG, "Could not read the installed PAK version.", exception);
            return false;
        }
    }

    private void writeVersionMarker(File versionMarker, long versionCode)
        throws IOException {

        File temporaryFile = new File(
            pakDirectory, PAK_VERSION_MARKER_NAME + ".part");
        deleteQuietly(temporaryFile);

        byte[] value = Long.toString(versionCode).getBytes(StandardCharsets.US_ASCII);
        try (FileOutputStream output = new FileOutputStream(temporaryFile, false)) {
            output.write(value);
            output.flush();
            output.getFD().sync();
        }

        replaceFile(temporaryFile, versionMarker);
    }

    private void removeOldPaks(File currentPak) {
        File[] files = pakDirectory.listFiles();
        if (files == null) {
            return;
        }

        for (File file : files) {
            String fileName = file.getName().toLowerCase(Locale.ROOT);
            if (!file.equals(currentPak) && file.isFile()
                && (fileName.endsWith(".pak") || fileName.endsWith(".part")
                    || fileName.endsWith(".bak"))) {
                deleteQuietly(file);
            }
        }
    }

    private String getApplicationName() {
        try {
            CharSequence label = getApplicationInfo().loadLabel(getPackageManager());
            if (label != null && !label.toString().trim().isEmpty()) {
                return label.toString();
            }
        } catch (Exception exception) {
            Log.w(TAG, "Could not read the application label.", exception);
        }
        return "OpenBOR";
    }

    private void startGameActivity() {
        if (gameStarted || isFinishing() || isDestroyed()) {
            return;
        }

        gameStarted = true;
        startActivity(new Intent(this, GameActivity.class));
        finish();
    }

    private void showPackagingError() {
        showFatalError(
            "This APK does not contain game data. Add the game package as "
                + "app/src/main/assets/bor.pak and rebuild.");
    }

    private void showStorageError(String detail) {
        showFatalError(applicationName + " could not prepare its game data. " + detail);
    }

    private void showFatalError(String message) {
        if (isFinishing() || isDestroyed()) {
            return;
        }

        new AlertDialog.Builder(this)
            .setTitle(applicationName)
            .setMessage(message)
            .setPositiveButton("Exit", (dialog, which) -> finish())
            .setCancelable(false)
            .show();
    }

    private String safeMessage(Exception exception) {
        String message = exception.getMessage();
        return message == null || message.trim().isEmpty()
            ? "No additional error information is available."
            : message;
    }

    private void deleteQuietly(File file) {
        if (file.isFile() && !file.delete()) {
            Log.w(TAG, "Could not delete " + file.getAbsolutePath());
        }
    }

    @Override
    protected void onDestroy() {
        installExecutor.shutdownNow();
        super.onDestroy();
    }
}
