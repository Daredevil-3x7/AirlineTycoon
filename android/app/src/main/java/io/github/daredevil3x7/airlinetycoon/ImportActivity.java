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
import java.io.FileInputStream;
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
import java.util.zip.ZipOutputStream;

/**
 * Start screen and settings. The APK contains no game data: on first start the player selects
 * a ZIP of their own Airline Tycoon Deluxe installation, which is unpacked into the app's
 * external files directory (Android/data/&lt;package&gt;/files). After that the game starts
 * directly; the settings are reached through the app shortcut (long press on the app icon).
 */
public class ImportActivity extends Activity {
    private static final String TAG = "ATImport";
    private static final int REQUEST_PICK_ZIP = 1;
    private static final int REQUEST_EXPORT_SAVES = 2;
    private static final int REQUEST_IMPORT_SAVES = 3;
    /** Sent by the app shortcut. */
    static final String ACTION_SETTINGS = "io.github.daredevil3x7.airlinetycoon.SETTINGS";

    private static final String PREFS = "at";
    private static final String PREF_LANGUAGE = "language";
    private static final String PREF_EXTRAS = "extras";
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
    private static final String SAVE_DIR = "savegame";

    private final Handler ui = new Handler(Looper.getMainLooper());
    private File dataDir;
    private TextView status;
    private Button startButton;
    private Button pickButton;
    private Button languageButton;
    private Button extrasButton;
    private Button exportButton;
    private Button importButton;
    private ProgressBar progress;
    private boolean busy;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        dataDir = getExternalFilesDir(null);
        buildUi();

        if (dataDir == null) {
            setButtonsEnabled(false);
            status.setText(R.string.import_no_storage);
            return;
        }
        boolean settings = ACTION_SETTINGS.equals(getIntent().getAction());
        if (!settings && hasGameData(dataDir)) {
            launchGame();
        }
    }

    // ---------------------------------------------------------------------------------------
    // UI
    // ---------------------------------------------------------------------------------------

    private int dp(int value) {
        return (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, value, getResources().getDisplayMetrics());
    }

    private Button addButton(LinearLayout column, View.OnClickListener listener) {
        Button button = new Button(this);
        button.setOnClickListener(listener);
        column.addView(button, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT));
        return button;
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

        boolean hasData = hasGameData(dataDir);

        TextView intro = new TextView(this);
        intro.setText(hasData ? R.string.settings_intro : R.string.import_intro);
        intro.setTextSize(TypedValue.COMPLEX_UNIT_SP, 16);
        intro.setPadding(0, dp(16), 0, dp(24));
        column.addView(intro);

        startButton = addButton(column, new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                launchGame();
            }
        });
        startButton.setText(R.string.settings_start);
        startButton.setVisibility(hasData ? View.VISIBLE : View.GONE);

        pickButton = addButton(column, new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                pickDocument(REQUEST_PICK_ZIP);
            }
        });
        pickButton.setText(hasData ? R.string.import_pick_again : R.string.import_pick);

        languageButton = addButton(column, new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                chooseLanguage();
            }
        });

        extrasButton = addButton(column, new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                SharedPreferences prefs = getSharedPreferences(PREFS, MODE_PRIVATE);
                prefs.edit().putBoolean(PREF_EXTRAS, !prefs.getBoolean(PREF_EXTRAS, true)).apply();
                updateLabels();
            }
        });

        exportButton = addButton(column, new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
                intent.addCategory(Intent.CATEGORY_OPENABLE);
                intent.setType("application/zip");
                intent.putExtra(Intent.EXTRA_TITLE, "AirlineTycoon-savegames.zip");
                startActivityForResult(intent, REQUEST_EXPORT_SAVES);
            }
        });
        exportButton.setText(R.string.saves_export);
        exportButton.setVisibility(hasData ? View.VISIBLE : View.GONE);

        importButton = addButton(column, new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                pickDocument(REQUEST_IMPORT_SAVES);
            }
        });
        importButton.setText(R.string.saves_import);
        importButton.setVisibility(hasData ? View.VISIBLE : View.GONE);

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

        updateLabels();

        ScrollView scroll = new ScrollView(this);
        scroll.addView(column);
        setContentView(scroll);
    }

    private void updateLabels() {
        languageButton.setText(getString(R.string.language_button, languageName()));
        extrasButton.setText(areExtrasEnabled(this) ? R.string.extras_on : R.string.extras_off);
    }

    private void setButtonsEnabled(boolean enabled) {
        for (Button button : new Button[] {startButton, pickButton, languageButton, extrasButton, exportButton, importButton}) {
            button.setEnabled(enabled);
        }
    }

    private void setBusy(boolean nowBusy) {
        busy = nowBusy;
        setButtonsEnabled(!nowBusy);
        progress.setVisibility(nowBusy ? View.VISIBLE : View.GONE);
        if (nowBusy) {
            getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        } else {
            getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        }
    }

    private void pickDocument(int request) {
        if (busy) {
            return;
        }
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        // Some file managers report ZIP files with a generic type, so do not filter too strictly
        intent.setType("*/*");
        intent.putExtra(Intent.EXTRA_MIME_TYPES,
                new String[] {"application/zip", "application/x-zip-compressed", "application/octet-stream"});
        startActivityForResult(intent, request);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (resultCode != RESULT_OK || data == null || data.getData() == null) {
            return;
        }
        if (requestCode == REQUEST_PICK_ZIP) {
            runTask(data.getData(), requestCode);
        } else if (requestCode == REQUEST_EXPORT_SAVES || requestCode == REQUEST_IMPORT_SAVES) {
            runTask(data.getData(), requestCode);
        }
    }

    @Override
    public void onBackPressed() {
        if (!busy) {
            super.onBackPressed();
        }
    }

    // ---------------------------------------------------------------------------------------
    // Settings
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

    /** Additional game rules of the free game: events, two start gates, sabotage by mood. */
    static boolean areExtrasEnabled(Context context) {
        return context.getSharedPreferences(PREFS, MODE_PRIVATE).getBoolean(PREF_EXTRAS, true);
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

    private void chooseLanguage() {
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
                        updateLabels();
                        dialog.dismiss();
                    }
                })
                .show();
    }

    // ---------------------------------------------------------------------------------------
    // Background work: game data import, savegame export and import
    // ---------------------------------------------------------------------------------------

    private void runTask(final Uri uri, final int request) {
        setBusy(true);
        status.setText(getString(R.string.import_running, 0, 0));

        new Thread(new Runnable() {
            @Override
            public void run() {
                String message;
                boolean ok = false;
                try {
                    if (request == REQUEST_PICK_ZIP) {
                        unzipGameData(uri);
                        ok = hasGameData(dataDir);
                        message = getString(ok ? R.string.import_done : R.string.import_not_game);
                    } else if (request == REQUEST_EXPORT_SAVES) {
                        int count = exportSaves(uri);
                        ok = count > 0;
                        message = ok ? getString(R.string.saves_exported, count) : getString(R.string.saves_none);
                    } else {
                        int count = importSaves(uri);
                        ok = count > 0;
                        message = ok ? getString(R.string.saves_imported, count) : getString(R.string.saves_none_in_zip);
                    }
                } catch (Exception e) {
                    Log.e(TAG, "Task failed", e);
                    message = getString(R.string.import_failed, String.valueOf(e.getMessage()));
                }
                final String text = message;
                final boolean launch = ok && request == REQUEST_PICK_ZIP;
                ui.post(new Runnable() {
                    @Override
                    public void run() {
                        setBusy(false);
                        status.setText(text);
                        if (launch) {
                            launchGame();
                        }
                    }
                });
            }
        }, "ATImport").start();
    }

    private InputStream open(Uri uri) throws IOException {
        InputStream raw = getContentResolver().openInputStream(uri);
        if (raw == null) {
            throw new IOException("cannot open file");
        }
        return new BufferedInputStream(raw, 1 << 16);
    }

    private static Charset zipCharset() {
        // ZIPs made by Windows store non-ASCII names in the OEM code page. Entries flagged as
        // UTF-8 are still decoded as UTF-8, whatever charset is passed.
        return Charset.isSupported("IBM437") ? Charset.forName("IBM437") : StandardCharsets.UTF_8;
    }

    private static long copy(InputStream in, OutputStream out, byte[] buffer) throws IOException {
        long total = 0;
        int n;
        while ((n = in.read(buffer)) > 0) {
            out.write(buffer, 0, n);
            total += n;
        }
        return total;
    }

    private void unzipGameData(Uri zip) throws IOException {
        String root = dataDir.getCanonicalPath() + File.separator;
        byte[] buffer = new byte[1 << 16];
        int files = 0;
        long bytes = 0;

        try (ZipInputStream in = new ZipInputStream(open(zip), zipCharset())) {
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
                    bytes += copy(in, out, buffer);
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
                if (busy) {
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

    /** Packs all files of the savegame folder into the chosen document. */
    private int exportSaves(Uri target) throws IOException {
        File dir = findIgnoreCase(dataDir, SAVE_DIR);
        File[] files = dir != null ? dir.listFiles() : null;
        if (files == null || files.length == 0) {
            return 0;
        }
        OutputStream raw = getContentResolver().openOutputStream(target);
        if (raw == null) {
            throw new IOException("cannot write file");
        }
        byte[] buffer = new byte[1 << 16];
        int count = 0;
        try (ZipOutputStream out = new ZipOutputStream(raw)) {
            for (File file : files) {
                if (!file.isFile()) {
                    continue;
                }
                out.putNextEntry(new ZipEntry(SAVE_DIR + "/" + file.getName()));
                try (InputStream in = new FileInputStream(file)) {
                    copy(in, out, buffer);
                }
                out.closeEntry();
                count++;
            }
        }
        return count;
    }

    /** Unpacks savegames (files inside a "savegame" folder of the ZIP) into the game's savegame folder. */
    private int importSaves(Uri zip) throws IOException {
        File dir = findIgnoreCase(dataDir, SAVE_DIR);
        if (dir == null) {
            dir = new File(dataDir, SAVE_DIR);
        }
        if (!dir.isDirectory() && !dir.mkdirs()) {
            throw new IOException("cannot create " + dir);
        }
        byte[] buffer = new byte[1 << 16];
        int count = 0;
        try (ZipInputStream in = new ZipInputStream(open(zip), zipCharset())) {
            ZipEntry entry;
            while ((entry = in.getNextEntry()) != null) {
                if (entry.isDirectory()) {
                    continue;
                }
                String[] parts = entry.getName().replace('\\', '/').split("/");
                String name = parts[parts.length - 1];
                boolean inSaveDir = parts.length >= 2 && parts[parts.length - 2].equalsIgnoreCase(SAVE_DIR);
                if (!inSaveDir || !name.matches("[A-Za-z0-9_.-]+") || name.startsWith(".")) {
                    continue;
                }
                try (OutputStream out = new FileOutputStream(new File(dir, name))) {
                    copy(in, out, buffer);
                }
                count++;
            }
        }
        return count;
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
        File[] children = dir != null ? dir.listFiles() : null;
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
                copy(in, out, buffer);
            }
        }
    }

    private void launchGame() {
        if (!hasGameData(dataDir)) {
            return;
        }
        try {
            copyPatchFiles();
        } catch (IOException e) {
            Log.e(TAG, "Could not copy patch files", e);
            status.setText(getString(R.string.import_failed, String.valueOf(e.getMessage())));
            return;
        }
        // left over from a test version that showed real-world headlines
        File oldNews = new File(dataDir, "news.txt");
        if (oldNews.exists() && !oldNews.delete()) {
            Log.w(TAG, "could not remove " + oldNews);
        }
        File oldMarker = new File(dataDir, "news.txt.lang");
        if (oldMarker.exists() && !oldMarker.delete()) {
            Log.w(TAG, "could not remove " + oldMarker);
        }
        startActivity(new Intent(this, ATActivity.class));
        finish();
    }
}
