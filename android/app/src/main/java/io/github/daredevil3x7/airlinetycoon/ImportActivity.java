package io.github.daredevil3x7.airlinetycoon;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.ScrollView;
import android.widget.TextView;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Locale;
import java.util.Set;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

/**
 * Start screen. The APK contains no game data: on first start the player selects a ZIP of
 * their own Airline Tycoon Deluxe installation, which is unpacked into the app's external
 * files directory (Android/data/&lt;package&gt;/files). After that the game starts directly.
 */
public class ImportActivity extends Activity {
    private static final String TAG = "ATImport";
    private static final int REQUEST_PICK_ZIP = 1;
    /** Sent by the app shortcut (long press on the app icon). */
    static final String ACTION_LANGUAGE = "io.github.daredevil3x7.airlinetycoon.LANGUAGE";

    private static final String PREFS = "at";
    private static final String PREF_LANGUAGE = "language";
    /** Names and the game's language numbers (LANGUAGE_... in defines.h). */
    private static final String[] LANGUAGE_NAMES =
            {"Deutsch", "English", "Français", "Polski", "Nederlands", "Italiano", "Español", "Português"};
    private static final int[] LANGUAGE_IDS = {0, 1, 2, 4, 5, 6, 7, 8};
    private static final String[] LANGUAGE_CODES = {"de", "en", "fr", "pl", "nl", "it", "es", "pt"};

    /** Top-level folders of the game. Everything above them inside the ZIP is stripped. */
    private static final Set<String> DATA_DIRS =
            new HashSet<>(Arrays.asList("data", "gli", "intro", "misc", "room", "sound", "video", "voice"));
    /** Without these the game cannot start. */
    private static final String[] REQUIRED_DIRS = {"data", "gli", "misc", "room"};

    private final Handler ui = new Handler(Looper.getMainLooper());
    private File dataDir;
    private TextView status;
    private Button pickButton;
    private ProgressBar progress;
    private Button languageButton;
    private boolean importing;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        dataDir = getExternalFilesDir(null);
        buildUi();

        if (dataDir == null) {
            pickButton.setEnabled(false);
            status.setText(R.string.import_no_storage);
            return;
        }
        if (ACTION_LANGUAGE.equals(getIntent().getAction())) {
            chooseLanguage(true);
        } else if (hasGameData(dataDir)) {
            launchGame();
        }
    }

    // ---------------------------------------------------------------------------------------
    // Language
    // ---------------------------------------------------------------------------------------

    /** The language number for the game: the player's choice, else the device language, else -1. */
    static int getLanguage(Context context) {
        SharedPreferences prefs = context.getSharedPreferences(PREFS, MODE_PRIVATE);
        if (prefs.contains(PREF_LANGUAGE)) {
            return prefs.getInt(PREF_LANGUAGE, -1);
        }
        String device = Locale.getDefault().getLanguage();
        for (int i = 0; i < LANGUAGE_CODES.length; i++) {
            if (LANGUAGE_CODES[i].equals(device)) {
                return LANGUAGE_IDS[i];
            }
        }
        return -1;
    }

    private String languageName() {
        int language = getLanguage(this);
        for (int i = 0; i < LANGUAGE_IDS.length; i++) {
            if (LANGUAGE_IDS[i] == language) {
                return LANGUAGE_NAMES[i];
            }
        }
        return LANGUAGE_NAMES[1];
    }

    private void chooseLanguage(final boolean launchAfterwards) {
        int language = getLanguage(this);
        int checked = 1;
        for (int i = 0; i < LANGUAGE_IDS.length; i++) {
            if (LANGUAGE_IDS[i] == language) {
                checked = i;
            }
        }
        new AlertDialog.Builder(this)
                .setTitle(R.string.language_title)
                .setSingleChoiceItems(LANGUAGE_NAMES, checked, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        getSharedPreferences(PREFS, MODE_PRIVATE).edit().putInt(PREF_LANGUAGE, LANGUAGE_IDS[which]).apply();
                        languageButton.setText(getString(R.string.language_button, languageName()));
                        dialog.dismiss();
                    }
                })
                .setOnDismissListener(new DialogInterface.OnDismissListener() {
                    @Override
                    public void onDismiss(DialogInterface dialog) {
                        if (launchAfterwards && hasGameData(dataDir)) {
                            launchGame();
                        }
                    }
                })
                .show();
    }

    // ---------------------------------------------------------------------------------------
    // UI
    // ---------------------------------------------------------------------------------------

    private int dp(int value) {
        return (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, value, getResources().getDisplayMetrics());
    }

    private void buildUi() {
        LinearLayout column = new LinearLayout(this);
        column.setOrientation(LinearLayout.VERTICAL);
        column.setGravity(Gravity.CENTER_HORIZONTAL);
        column.setPadding(dp(24), dp(32), dp(24), dp(32));

        TextView title = new TextView(this);
        title.setText(R.string.import_title);
        title.setTextSize(TypedValue.COMPLEX_UNIT_SP, 22);
        column.addView(title);

        TextView intro = new TextView(this);
        intro.setText(R.string.import_intro);
        intro.setTextSize(TypedValue.COMPLEX_UNIT_SP, 16);
        intro.setPadding(0, dp(16), 0, dp(24));
        column.addView(intro);

        pickButton = new Button(this);
        pickButton.setText(R.string.import_pick);
        pickButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                pickZip();
            }
        });
        column.addView(pickButton);

        languageButton = new Button(this);
        languageButton.setText(getString(R.string.language_button, languageName()));
        languageButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                chooseLanguage(false);
            }
        });
        column.addView(languageButton);

        progress = new ProgressBar(this);
        progress.setIndeterminate(true);
        progress.setVisibility(View.GONE);
        progress.setPadding(0, dp(24), 0, 0);
        column.addView(progress);

        status = new TextView(this);
        status.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        status.setPadding(0, dp(16), 0, 0);
        if (dataDir != null) {
            status.setText(getString(R.string.import_manual, dataDir.getAbsolutePath()));
        }
        column.addView(status);

        ScrollView scroll = new ScrollView(this);
        scroll.addView(column);
        setContentView(scroll);
    }

    private void pickZip() {
        if (importing) {
            return;
        }
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        // Some file managers report ZIP files with a generic type, so do not filter too strictly
        intent.setType("*/*");
        intent.putExtra(Intent.EXTRA_MIME_TYPES,
                new String[] {"application/zip", "application/x-zip-compressed", "application/octet-stream"});
        startActivityForResult(intent, REQUEST_PICK_ZIP);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == REQUEST_PICK_ZIP && resultCode == RESULT_OK && data != null && data.getData() != null) {
            startImport(data.getData());
        }
    }

    @Override
    public void onBackPressed() {
        if (!importing) {
            super.onBackPressed();
        }
    }

    // ---------------------------------------------------------------------------------------
    // Import
    // ---------------------------------------------------------------------------------------

    private void startImport(final Uri zip) {
        importing = true;
        pickButton.setEnabled(false);
        progress.setVisibility(View.VISIBLE);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        status.setText(getString(R.string.import_running, 0, 0));

        new Thread(new Runnable() {
            @Override
            public void run() {
                String error = null;
                try {
                    unzip(zip);
                    if (!hasGameData(dataDir)) {
                        error = getString(R.string.import_not_game);
                    }
                } catch (Exception e) {
                    Log.e(TAG, "Import failed", e);
                    error = getString(R.string.import_failed, String.valueOf(e.getMessage()));
                }
                final String result = error;
                ui.post(new Runnable() {
                    @Override
                    public void run() {
                        importFinished(result);
                    }
                });
            }
        }, "ATImport").start();
    }

    private void importFinished(String error) {
        importing = false;
        progress.setVisibility(View.GONE);
        getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        if (error != null) {
            pickButton.setEnabled(true);
            status.setText(error);
            return;
        }
        status.setText(R.string.import_done);
        launchGame();
    }

    private void unzip(Uri zip) throws IOException {
        // ZIPs made by Windows store non-ASCII names in the OEM code page. Entries flagged as
        // UTF-8 are still decoded as UTF-8, whatever charset is passed here.
        Charset charset = Charset.isSupported("IBM437") ? Charset.forName("IBM437") : StandardCharsets.UTF_8;
        String root = dataDir.getCanonicalPath() + File.separator;
        byte[] buffer = new byte[1 << 16];
        int files = 0;
        long bytes = 0;

        InputStream raw = getContentResolver().openInputStream(zip);
        if (raw == null) {
            throw new IOException("cannot open file");
        }
        try (ZipInputStream in = new ZipInputStream(new BufferedInputStream(raw, 1 << 16), charset)) {
            ZipEntry entry;
            while ((entry = in.getNextEntry()) != null) {
                String relative = entry.isDirectory() ? null : mapEntry(entry.getName());
                if (relative == null) {
                    continue;
                }
                File target = new File(dataDir, relative);
                if (!target.getCanonicalPath().startsWith(root)) {
                    continue; // never write outside of the data directory
                }
                File parent = target.getParentFile();
                if (parent != null && !parent.isDirectory() && !parent.mkdirs()) {
                    throw new IOException("cannot create " + parent);
                }
                try (OutputStream out = new FileOutputStream(target)) {
                    int n;
                    while ((n = in.read(buffer)) > 0) {
                        out.write(buffer, 0, n);
                        bytes += n;
                    }
                }
                files++;
                if (files % 20 == 0) {
                    reportProgress(files, bytes);
                }
            }
        }
        reportProgress(files, bytes);
    }

    private void reportProgress(final int files, final long bytes) {
        ui.post(new Runnable() {
            @Override
            public void run() {
                if (importing) {
                    status.setText(getString(R.string.import_running, files, (int) (bytes >> 20)));
                }
            }
        });
    }

    /**
     * Maps a ZIP entry to a path below the data directory: "Airline Tycoon Deluxe/gli/x.gli"
     * becomes "gli/x.gli". Files that are not inside one of the game's folders (the Windows
     * executable, DLLs, the manual) are skipped.
     */
    static String mapEntry(String name) {
        String[] parts = name.replace('\\', '/').split("/");
        int start = -1;
        for (int i = 0; i < parts.length; i++) {
            if (parts[i].equals("..")) {
                return null;
            }
            if (start < 0 && i < parts.length - 1 && DATA_DIRS.contains(parts[i].toLowerCase(Locale.ROOT))) {
                start = i;
            }
        }
        if (start < 0) {
            return null;
        }
        StringBuilder path = new StringBuilder();
        for (int i = start; i < parts.length; i++) {
            if (parts[i].isEmpty() || parts[i].equals(".")) {
                continue;
            }
            if (path.length() > 0) {
                path.append('/');
            }
            // folders in lower case, so that a second import cannot create "SOUND" next to "sound"
            path.append(i < parts.length - 1 ? parts[i].toLowerCase(Locale.ROOT) : parts[i]);
        }
        return path.toString();
    }

    // ---------------------------------------------------------------------------------------
    // Game start
    // ---------------------------------------------------------------------------------------

    static boolean hasGameData(File dir) {
        if (dir == null) {
            return false;
        }
        for (String required : REQUIRED_DIRS) {
            File child = findIgnoreCase(dir, required);
            String[] content = child != null && child.isDirectory() ? child.list() : null;
            if (content == null || content.length == 0) {
                return false;
            }
        }
        return true;
    }

    private static File findIgnoreCase(File dir, String name) {
        File[] children = dir.listFiles();
        if (children != null) {
            for (File child : children) {
                if (child.getName().equalsIgnoreCase(name)) {
                    return child;
                }
            }
        }
        return null;
    }

    /** The patched text resources ship inside the APK and are refreshed on every start. */
    private void copyPatchFiles() throws IOException {
        String[] names = getAssets().list("patch");
        if (names == null || names.length == 0) {
            return;
        }
        File patchDir = new File(dataDir, "patch");
        if (!patchDir.isDirectory() && !patchDir.mkdirs()) {
            throw new IOException("cannot create " + patchDir);
        }
        byte[] buffer = new byte[1 << 16];
        for (String name : names) {
            try (InputStream in = getAssets().open("patch/" + name);
                 OutputStream out = new FileOutputStream(new File(patchDir, name))) {
                int n;
                while ((n = in.read(buffer)) > 0) {
                    out.write(buffer, 0, n);
                }
            }
        }
    }

    private void launchGame() {
        try {
            copyPatchFiles();
        } catch (IOException e) {
            Log.e(TAG, "Could not copy patch files", e);
            status.setText(getString(R.string.import_failed, String.valueOf(e.getMessage())));
            return;
        }
        startActivity(new Intent(this, ATActivity.class));
        finish();
    }
}
