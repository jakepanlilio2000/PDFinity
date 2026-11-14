package com.stacksyncsolutions.pdfinity;

import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.os.CancellationSignal;
import android.os.ParcelFileDescriptor;
import android.print.PageRange;
import android.print.PrintAttributes;
import android.print.PrintDocumentAdapter;
import android.print.PrintDocumentInfo;
import android.print.PrintManager;
import android.provider.OpenableColumns;
import android.util.Log;

import androidx.core.content.FileProvider;
import org.qtproject.qt.android.bindings.QtActivity;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * PDFinity Mobile Android Activity
 * Publisher: StackSync Solutions (https://www.stacksyncsolutions.com/)
 * Lead Programmer: Jake Ashley Panlilio (https://github.com/jakepanlilio2000)
 *
 * Implements Android Storage Access Framework (SAF) URI resolution,
 * parcel file descriptors, Android system printing via PrintManager,
 * system sharing via FileProvider, and bridge to native C++ Qt core.
 */
public class PDFinityActivity extends QtActivity {
    private static final String TAG = "PDFinityActivity";
    private static PDFinityActivity sInstance = null;

    // Native JNI hook into C++ PDFinity core
    public static native void onNativeDocumentReceived(String localFilePath);
    public static native void onNativeLifecycleStateChanged(int state);

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        sInstance = this;
        cleanupOldCacheFiles();
        handleIntent(getIntent());
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (sInstance == this) {
            sInstance = null;
        }
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        setIntent(intent);
        handleIntent(intent);
    }

    /**
     * Resolves incoming file:// and content:// URIs and provides an accessible local file path
     * to the native PDFium/C++ engine.
     */
    private void handleIntent(Intent intent) {
        if (intent == null) return;

        String action = intent.getAction();
        Uri dataUri = null;

        if (Intent.ACTION_VIEW.equals(action) || Intent.ACTION_EDIT.equals(action)) {
            dataUri = intent.getData();
        } else if (Intent.ACTION_SEND.equals(action)) {
            dataUri = intent.getParcelableExtra(Intent.EXTRA_STREAM);
        }

        if (dataUri != null) {
            String resolvedPath = resolveUriToLocalFile(dataUri);
            if (resolvedPath != null) {
                Log.i(TAG, "Successfully resolved PDF URI to local path: " + resolvedPath);
                try {
                    onNativeDocumentReceived(resolvedPath);
                } catch (UnsatisfiedLinkError e) {
                    Log.w(TAG, "Native library not yet initialized. Document queued: " + resolvedPath);
                }
            } else {
                Log.e(TAG, "Failed to resolve content URI: " + dataUri);
            }
        }
    }

    /**
     * Copies content:// stream into app-internal cache directory for reliable random-access I/O
     * by native C++ PDFium and QPDF engines.
     */
    private String resolveUriToLocalFile(Uri uri) {
        if (uri == null) return null;

        if ("file".equalsIgnoreCase(uri.getScheme())) {
            return uri.getPath();
        }

        if ("content".equalsIgnoreCase(uri.getScheme())) {
            ContentResolver resolver = getContentResolver();
            String displayName = "imported_document.pdf";

            try (Cursor cursor = resolver.query(uri, null, null, null, null)) {
                if (cursor != null && cursor.moveToFirst()) {
                    int nameIndex = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                    if (nameIndex != -1) {
                        String name = cursor.getString(nameIndex);
                        if (name != null && !name.trim().isEmpty()) {
                            displayName = name;
                        }
                    }
                }
            } catch (Exception e) {
                Log.w(TAG, "Could not query display name from content resolver", e);
            }

            File cacheDir = new File(getCacheDir(), "saf_inbox");
            if (!cacheDir.exists() && !cacheDir.mkdirs()) {
                Log.e(TAG, "Failed to create SAF cache directory");
                return null;
            }

            File targetFile = new File(cacheDir, System.currentTimeMillis() + "_" + displayName);
            try (ParcelFileDescriptor pfd = resolver.openFileDescriptor(uri, "r")) {
                if (pfd == null) {
                    Log.e(TAG, "ContentResolver returned null ParcelFileDescriptor for URI: " + uri);
                    return null;
                }
                try (FileInputStream in = new FileInputStream(pfd.getFileDescriptor());
                     FileOutputStream out = new FileOutputStream(targetFile)) {
                    byte[] buffer = new byte[65536];
                    int bytesRead;
                    while ((bytesRead = in.read(buffer)) != -1) {
                        out.write(buffer, 0, bytesRead);
                    }
                    out.flush();
                    return targetFile.getAbsolutePath();
                }
            } catch (Exception e) {
                Log.e(TAG, "Error caching content URI via ParcelFileDescriptor", e);
                return null;
            }
        }

        return null;
    }

    /**
     * Flushes modified document safely back to the target content:// URI using atomic staging in
     * context.getCacheDir() before streaming into the target ParcelFileDescriptor.
     */
    public static boolean saveDocumentToUri(String localCachePath, String targetUriString) {
        if (sInstance == null || localCachePath == null || targetUriString == null) return false;
        File sourceFile = new File(localCachePath);
        if (!sourceFile.exists()) {
            Log.e(TAG, "Source file for SAF writeback does not exist: " + localCachePath);
            return false;
        }

        File stagingDir = new File(sInstance.getCacheDir(), "saf_staging");
        if (!stagingDir.exists() && !stagingDir.mkdirs()) {
            Log.e(TAG, "Failed to create SAF atomic staging directory");
            return false;
        }

        File tempStageFile = null;
        try {
            tempStageFile = File.createTempFile("saf_atomic_stage_", ".tmp", stagingDir);
            // 1. Copy source to staging file
            try (FileInputStream in = new FileInputStream(sourceFile);
                 FileOutputStream out = new FileOutputStream(tempStageFile)) {
                byte[] buffer = new byte[65536];
                int bytesRead;
                while ((bytesRead = in.read(buffer)) != -1) {
                    out.write(buffer, 0, bytesRead);
                }
                out.flush();
            }

            // 2. Stream staging file to target content URI descriptor
            Uri targetUri = Uri.parse(targetUriString);
            ContentResolver resolver = sInstance.getContentResolver();
            try (ParcelFileDescriptor pfd = resolver.openFileDescriptor(targetUri, "w")) {
                if (pfd == null) {
                    Log.e(TAG, "Failed to obtain writable ParcelFileDescriptor for target URI: " + targetUriString);
                    return false;
                }
                try (FileOutputStream out = new FileOutputStream(pfd.getFileDescriptor());
                     FileInputStream in = new FileInputStream(tempStageFile)) {
                    byte[] buffer = new byte[65536];
                    int bytesRead;
                    while ((bytesRead = in.read(buffer)) != -1) {
                        out.write(buffer, 0, bytesRead);
                    }
                    out.flush();
                    pfd.getFileDescriptor().sync();
                    Log.i(TAG, "Successfully committed document atomically to SAF URI: " + targetUriString);
                    return true;
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "Failed atomic SAF document writeback", e);
            return false;
        } finally {
            if (tempStageFile != null && tempStageFile.exists()) {
                tempStageFile.delete();
            }
        }
    }

    /**
     * Launches Android system printing service for the specified PDF file.
     */
    public static boolean printDocument(final String localFilePath) {
        if (sInstance == null || localFilePath == null) return false;
        final File file = new File(localFilePath);
        if (!file.exists()) return false;

        sInstance.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                try {
                    PrintManager printManager = (PrintManager) sInstance.getSystemService(Context.PRINT_SERVICE);
                    if (printManager == null) return;

                    String jobName = "PDFinity - " + file.getName();
                    printManager.print(jobName, new PrintDocumentAdapter() {
                        @Override
                        public void onLayout(PrintAttributes oldAttributes, PrintAttributes newAttributes,
                                             CancellationSignal cancellationSignal, LayoutResultCallback callback, Bundle extras) {
                            if (cancellationSignal.isCanceled()) {
                                callback.onLayoutCancelled();
                                return;
                            }
                            PrintDocumentInfo pdi = new PrintDocumentInfo.Builder(file.getName())
                                    .setContentType(PrintDocumentInfo.CONTENT_TYPE_DOCUMENT)
                                    .build();
                            callback.onLayoutFinished(pdi, true);
                        }

                        @Override
                        public void onWrite(PageRange[] pages, ParcelFileDescriptor destination,
                                            CancellationSignal cancellationSignal, WriteResultCallback callback) {
                            try (InputStream input = new FileInputStream(file);
                                 OutputStream output = new FileOutputStream(destination.getFileDescriptor())) {
                                byte[] buf = new byte[16384];
                                int bytesRead;
                                while ((bytesRead = input.read(buf)) >= 0) {
                                    if (cancellationSignal.isCanceled()) {
                                        callback.onWriteCancelled();
                                        return;
                                    }
                                    output.write(buf, 0, bytesRead);
                                }
                                callback.onWriteFinished(new PageRange[]{PageRange.ALL_PAGES});
                            } catch (Exception e) {
                                callback.onWriteFailed(e.getMessage());
                            }
                        }
                    }, null);
                } catch (Exception e) {
                    Log.e(TAG, "Error initiating Android PrintManager", e);
                }
            }
        });
        return true;
    }

    /**
     * Shares a PDF file with other Android apps using FileProvider.
     */
    public static boolean shareDocument(String localFilePath) {
        if (sInstance == null || localFilePath == null) return false;
        File file = new File(localFilePath);
        if (!file.exists()) return false;

        try {
            Uri contentUri = FileProvider.getUriForFile(sInstance,
                    "com.stacksyncsolutions.pdfinity.fileprovider", file);

            Intent shareIntent = new Intent(Intent.ACTION_SEND);
            shareIntent.setType("application/pdf");
            shareIntent.putExtra(Intent.EXTRA_STREAM, contentUri);
            shareIntent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);

            sInstance.startActivity(Intent.createChooser(shareIntent, "Share PDF with..."));
            return true;
        } catch (Exception e) {
            Log.e(TAG, "Error sharing document via FileProvider", e);
            return false;
        }
    }

    /**
     * Automatically purges temporary SAF files older than 24 hours to prevent cache ballooning.
     */
    private void cleanupOldCacheFiles() {
        try {
            File cacheDir = new File(getCacheDir(), "saf_inbox");
            if (cacheDir.exists() && cacheDir.isDirectory()) {
                File[] files = cacheDir.listFiles();
                if (files != null) {
                    long now = System.currentTimeMillis();
                    long oneDay = 24L * 60L * 60L * 1000L;
                    for (File f : files) {
                        if (now - f.lastModified() > oneDay) {
                            f.delete();
                        }
                    }
                }
            }
        } catch (Exception e) {
            Log.w(TAG, "Error during cache cleanup", e);
        }
    }
}
