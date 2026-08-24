package org.openbor.engine;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.ContentResolver;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.provider.OpenableColumns;
import android.util.Log;
import android.widget.Toast;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Locale;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/*
- Caskey, Damon V.
- 2026-08-24
-
- Route existing PAK files to OpenBOR's native module selector, import a PAK only
- when storage is empty, and restore bundled bor.pak support for dedicated APKs.
*/
public class LauncherActivity extends Activity {

    private static final String TAG = "LauncherActivity";
    private static final int PICK_PAK_FILE_REQUEST_CODE = 1;
    private static final int COPY_BUFFER_SIZE = 64 * 1024;
    private static final String GENERIC_PACKAGE_NAME = "org.openbor.engine";
    private static final String PAK_DIRECTORY_NAME = "Paks";
    private static final String BUNDLED_PAK_NAME = "bor.pak";

    private final ExecutorService copyExecutor = Executors.newSingleThreadExecutor();
    private File pakDirectory;
    private ProgressDialog progressDialog;
    private boolean gameStarted;
    private boolean pickerOpen;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        File externalFilesDirectory = getExternalFilesDir(null);
        if (externalFilesDirectory == null) {
            showFatalError("OpenBOR could not access its application storage.");
            return;
        }

        pakDirectory = new File(externalFilesDirectory, PAK_DIRECTORY_NAME);
        if (!pakDirectory.isDirectory() && !pakDirectory.mkdirs()) {
            showFatalError("OpenBOR could not create its Paks folder.");
            return;
        }

        preparePaksAndContinue();
    }

    /*
    - Caskey, Damon V.
    - 2026-08-24
    -
    - Dedicated APKs install their bundled asset before PAK count is evaluated.
    - Generic builds launch native selection when one or more PAKs already exist.
    */
    private void preparePaksAndContinue() {
        copyExecutor.execute(() -> {
            try {
                installBundledPakForDedicatedBuild();
                int pakCount = countInstalledPaks();
                Log.i(TAG, "Installed PAK count: " + pakCount);

                runOnUiThread(() -> {
                    if (pakCount == 0) {
                        showPakSelectionDialog();
                    } else {
                        startGameActivity();
                    }
                });
            } catch (Exception exception) {
                Log.e(TAG, "Could not prepare PAK storage.", exception);
                runOnUiThread(() -> showRetryDialog(
                    "OpenBOR could not prepare its game data. " + safeMessage(exception),
                    this::preparePaksAndContinue));
            }
        });
    }

    private void showPakSelectionDialog() {
        if (pickerOpen || gameStarted) {
            return;
        }

        pickerOpen = true;
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        intent.addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);

        try {
            startActivityForResult(intent, PICK_PAK_FILE_REQUEST_CODE);
        } catch (Exception exception) {
            pickerOpen = false;
            Log.e(TAG, "Could not launch the Android file picker.", exception);
            showFatalError("OpenBOR could not open a file picker. Please install or enable a file manager.");
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if (requestCode != PICK_PAK_FILE_REQUEST_CODE) {
            return;
        }

        pickerOpen = false;
        if (resultCode == Activity.RESULT_CANCELED) {
            if (countInstalledPaks() > 0) {
                startGameActivity();
            } else {
                finish();
            }
            return;
        }

        if (resultCode != Activity.RESULT_OK || data == null || data.getData() == null) {
            showRetryDialog("OpenBOR could not read the selected file.", this::showPakSelectionDialog);
            return;
        }

        Uri pakUri = data.getData();
        String fileName = getSafeFileName(pakUri);
        if (!isPakFileName(fileName)) {
            showRetryDialog("Please select a file with the .pak extension.", this::showPakSelectionDialog);
            return;
        }

        persistReadPermission(pakUri, data.getFlags());
        copySelectedPakFile(pakUri, fileName, getFileSize(pakUri));
    }

    /*
    - Caskey, Damon V.
    - 2026-08-24
    -
    - Copy on a worker thread and publish only a fully closed, size-checked file.
    */
    private void copySelectedPakFile(Uri pakUri, String fileName, long expectedSize) {
        showProgress("Importing " + fileName + "...");

        copyExecutor.execute(() -> {
            File destinationFile = new File(pakDirectory, fileName);
            File temporaryFile = new File(pakDirectory, fileName + ".part");

            try {
                ContentResolver resolver = getContentResolver();
                try (InputStream input = resolver.openInputStream(pakUri)) {
                    if (input == null) {
                        throw new IOException("The selected file could not be opened.");
                    }
                    copyToTemporaryFile(input, temporaryFile, expectedSize);
                }

                replaceFile(temporaryFile, destinationFile);
                Log.i(TAG, "Imported PAK: " + destinationFile.getAbsolutePath());
                runOnUiThread(() -> {
                    dismissProgress();
                    Toast.makeText(this, "Imported " + fileName + ".", Toast.LENGTH_SHORT).show();
                    startGameActivity();
                });
            } catch (Exception exception) {
                deleteQuietly(temporaryFile);
                Log.e(TAG, "Could not import PAK " + fileName + ".", exception);
                runOnUiThread(() -> {
                    dismissProgress();
                    showRetryDialog(
                        "OpenBOR could not import " + fileName + ". " + safeMessage(exception),
                        this::showPakSelectionDialog);
                });
            }
        });
    }

    private void installBundledPakForDedicatedBuild() throws Exception {
        if (GENERIC_PACKAGE_NAME.equals(getPackageName())) {
            return;
        }

        String versionName = getVersionName();
        File destinationFile = new File(pakDirectory, safeVersionFileName(versionName) + ".pak");
        if (destinationFile.isFile() && destinationFile.length() > 0) {
            removeOldDedicatedPaks(destinationFile);
            return;
        }

        File temporaryFile = new File(pakDirectory, destinationFile.getName() + ".part");
        try (InputStream input = getAssets().open(BUNDLED_PAK_NAME)) {
            copyToTemporaryFile(input, temporaryFile, -1);
        } catch (Exception exception) {
            deleteQuietly(temporaryFile);
            throw exception;
        }

        replaceFile(temporaryFile, destinationFile);
        removeOldDedicatedPaks(destinationFile);
        Log.i(TAG, "Installed bundled PAK: " + destinationFile.getAbsolutePath());
    }

    private String getVersionName() throws Exception {
        PackageInfo packageInfo = getPackageManager().getPackageInfo(getPackageName(), 0);
        if (packageInfo.versionName == null || packageInfo.versionName.trim().isEmpty()) {
            return "bor";
        }
        return packageInfo.versionName;
    }

    private String safeVersionFileName(String versionName) {
        return versionName.replaceAll("[^A-Za-z0-9._-]", "_");
    }

    private void removeOldDedicatedPaks(File currentPak) {
        File[] files = pakDirectory.listFiles();
        if (files == null) {
            return;
        }

        for (File file : files) {
            if (!file.equals(currentPak) && file.isFile() && isPakFileName(file.getName())) {
                deleteQuietly(file);
            }
        }
    }

    private int countInstalledPaks() {
        if (pakDirectory == null) {
            return 0;
        }

        File[] files = pakDirectory.listFiles(file ->
            file.isFile() && file.length() > 0 && isPakFileName(file.getName()));
        return files == null ? 0 : files.length;
    }

    private boolean isPakFileName(String fileName) {
        return fileName != null && fileName.toLowerCase(Locale.ROOT).endsWith(".pak");
    }

    private String getSafeFileName(Uri uri) {
        String result = null;
        if ("content".equals(uri.getScheme())) {
            try (Cursor cursor = getContentResolver().query(
                uri,
                new String[] {OpenableColumns.DISPLAY_NAME},
                null,
                null,
                null)) {
                if (cursor != null && cursor.moveToFirst()) {
                    int nameIndex = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                    if (nameIndex >= 0) {
                        result = cursor.getString(nameIndex);
                    }
                }
            } catch (Exception exception) {
                Log.w(TAG, "Could not query the selected file name.", exception);
            }
        }

        if (result == null || result.trim().isEmpty()) {
            result = uri.getLastPathSegment();
        }
        if (result == null) {
            return null;
        }

        result = result.replace('\\', '/');
        int separator = result.lastIndexOf('/');
        return separator >= 0 ? result.substring(separator + 1) : result;
    }

    private long getFileSize(Uri uri) {
        try (Cursor cursor = getContentResolver().query(
            uri,
            new String[] {OpenableColumns.SIZE},
            null,
            null,
            null)) {
            if (cursor != null && cursor.moveToFirst()) {
                int sizeIndex = cursor.getColumnIndex(OpenableColumns.SIZE);
                if (sizeIndex >= 0 && !cursor.isNull(sizeIndex)) {
                    return cursor.getLong(sizeIndex);
                }
            }
        } catch (Exception exception) {
            Log.w(TAG, "Could not query the selected file size.", exception);
        }
        return -1;
    }

    private void persistReadPermission(Uri uri, int resultFlags) {
        int readFlag = resultFlags & Intent.FLAG_GRANT_READ_URI_PERMISSION;
        if (readFlag == 0) {
            return;
        }

        try {
            getContentResolver().takePersistableUriPermission(uri, readFlag);
        } catch (SecurityException exception) {
            // The transient grant is enough for the immediate background copy.
            Log.w(TAG, "The selected provider did not grant persistent read access.", exception);
        }
    }

    private void copyToTemporaryFile(InputStream input, File temporaryFile, long expectedSize)
        throws IOException {
        deleteQuietly(temporaryFile);
        long copiedSize = 0;

        try (FileOutputStream output = new FileOutputStream(temporaryFile)) {
            byte[] buffer = new byte[COPY_BUFFER_SIZE];
            int read;
            while ((read = input.read(buffer)) != -1) {
                output.write(buffer, 0, read);
                copiedSize += read;
            }
            output.flush();
            output.getFD().sync();
        }

        if (copiedSize == 0) {
            throw new IOException("The selected PAK is empty.");
        }
        if (expectedSize >= 0 && copiedSize != expectedSize) {
            throw new IOException(
                "The copy was incomplete (expected " + expectedSize + " bytes, copied " + copiedSize + ").");
        }
    }

    private void replaceFile(File temporaryFile, File destinationFile) throws IOException {
        File backupFile = new File(pakDirectory, destinationFile.getName() + ".bak");
        deleteQuietly(backupFile);

        boolean hadDestination = destinationFile.exists();
        if (hadDestination && !destinationFile.renameTo(backupFile)) {
            throw new IOException("The existing PAK could not be replaced.");
        }

        if (!temporaryFile.renameTo(destinationFile)) {
            if (hadDestination && !backupFile.renameTo(destinationFile)) {
                Log.e(TAG, "Could not restore the previous PAK after an import failure.");
            }
            throw new IOException("The imported PAK could not be installed.");
        }

        deleteQuietly(backupFile);
    }

    private void deleteQuietly(File file) {
        if (file.exists() && !file.delete()) {
            Log.w(TAG, "Could not delete " + file.getAbsolutePath());
        }
    }

    private void startGameActivity() {
        if (gameStarted) {
            return;
        }

        gameStarted = true;
        startActivity(new Intent(this, GameActivity.class));
        finish();
    }

    private void showProgress(String message) {
        dismissProgress();
        progressDialog = new ProgressDialog(this);
        progressDialog.setIndeterminate(true);
        progressDialog.setCancelable(false);
        progressDialog.setMessage(message);
        progressDialog.show();
    }

    private void dismissProgress() {
        if (progressDialog != null) {
            progressDialog.dismiss();
            progressDialog = null;
        }
    }

    private void showRetryDialog(String message, Runnable retryAction) {
        if (isFinishing()) {
            return;
        }

        new AlertDialog.Builder(this)
            .setTitle("OpenBOR")
            .setMessage(message)
            .setPositiveButton("Retry", (dialog, which) -> retryAction.run())
            .setNegativeButton("Exit", (dialog, which) -> finish())
            .setCancelable(false)
            .show();
    }

    private void showFatalError(String message) {
        Log.e(TAG, message);
        if (isFinishing()) {
            return;
        }

        new AlertDialog.Builder(this)
            .setTitle("OpenBOR")
            .setMessage(message)
            .setPositiveButton("Exit", (dialog, which) -> finish())
            .setCancelable(false)
            .show();
    }

    private String safeMessage(Exception exception) {
        String message = exception.getMessage();
        return message == null || message.trim().isEmpty() ? "Please try again." : message;
    }

    @Override
    protected void onDestroy() {
        dismissProgress();
        copyExecutor.shutdownNow();
        super.onDestroy();
    }
}
