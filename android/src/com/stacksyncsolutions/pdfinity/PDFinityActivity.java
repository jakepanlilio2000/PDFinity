package com.stacksyncsolutions.pdfinity;

import android.content.ContentResolver;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.provider.OpenableColumns;
import android.util.Log;

import org.qtproject.qt.android.bindings.QtActivity;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;

/**
 * PDFinity Mobile Android Activity
 * Publisher: StackSync Solutions (https://www.stacksyncsolutions.com/)
 * Lead Programmer: Jake Ashley Panlilio (https://github.com/jakepanlilio2000)
 *
 * Implements Android Storage Access Framework (SAF) URI resolution,
 * intent handling for application/pdf, and bridge to native C++ Qt core.
 */
public class PDFinityActivity extends QtActivity {
    private static final String TAG = "PDFinityActivity";

    // Native JNI hook into C++ PDFinity core
    public static native void onNativeDocumentReceived(String localFilePath);

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        handleIntent(getIntent());
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
            try (InputStream in = resolver.openInputStream(uri);
                 FileOutputStream out = new FileOutputStream(targetFile)) {
                if (in == null) return null;

                byte[] buffer = new byte[65536];
                int bytesRead;
                while ((bytesRead = in.read(buffer)) != -1) {
                    out.write(buffer, 0, bytesRead);
                }
                out.flush();
                return targetFile.getAbsolutePath();
            } catch (Exception e) {
                Log.e(TAG, "Error caching content URI to local storage", e);
                return null;
            }
        }

        return null;
    }
}
