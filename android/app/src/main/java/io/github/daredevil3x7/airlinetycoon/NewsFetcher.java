package io.github.daredevil3x7.airlinetycoon;

import android.util.Log;
import android.util.Xml;

import org.xmlpull.v1.XmlPullParser;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStreamWriter;
import java.io.Writer;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;

/**
 * Loads current headlines for the newspapers in the game's kiosk and stores them as
 * "news.txt" (one headline per line, ISO-8859-1) in the game folder. Only the titles of a
 * public RSS feed are used.
 */
final class NewsFetcher {
    private static final String TAG = "ATNews";
    static final String FILE_NAME = "news.txt";
    private static final String FEED_GERMAN = "https://www.tagesschau.de/index~rss2.xml";
    private static final String FEED_ENGLISH = "https://feeds.bbci.co.uk/news/world/rss.xml";
    private static final long MAX_AGE_MS = 2L * 60 * 60 * 1000;
    private static final int MAX_HEADLINES = 40;
    private static final int MAX_LENGTH = 110;

    private NewsFetcher() {}

    /** language: the game's language number (0 = German). Runs in the background. */
    static void refreshAsync(final File dataDir, final int language, final boolean enabled) {
        if (dataDir == null) {
            return;
        }
        final File target = new File(dataDir, FILE_NAME);
        if (!enabled) {
            if (target.exists() && !target.delete()) {
                Log.w(TAG, "could not remove " + target);
            }
            return;
        }
        final File marker = new File(dataDir, FILE_NAME + ".lang");
        final boolean sameLanguage = marker.exists() && marker.length() == (language == 0 ? 1 : 2);
        if (target.exists() && sameLanguage && System.currentTimeMillis() - target.lastModified() < MAX_AGE_MS) {
            return;
        }
        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    List<String> headlines = download(language == 0 ? FEED_GERMAN : FEED_ENGLISH, language == 0);
                    if (headlines.isEmpty()) {
                        return;
                    }
                    File temp = new File(dataDir, FILE_NAME + ".tmp");
                    try (Writer out = new OutputStreamWriter(new FileOutputStream(temp), StandardCharsets.ISO_8859_1)) {
                        for (String headline : headlines) {
                            out.write(headline);
                            out.write('\n');
                        }
                    }
                    if (!temp.renameTo(target)) {
                        Log.w(TAG, "could not replace " + target);
                    }
                    // remembers which feed the file is from: 1 byte = German, 2 bytes = English
                    try (FileOutputStream out = new FileOutputStream(marker)) {
                        out.write(language == 0 ? new byte[] {'d'} : new byte[] {'e', 'n'});
                    }
                } catch (Exception e) {
                    Log.w(TAG, "no headlines: " + e);
                }
            }
        }, "ATNews").start();
    }

    private static List<String> download(String address, boolean german) throws Exception {
        List<String> result = new ArrayList<>();
        HttpURLConnection connection = (HttpURLConnection) new URL(address).openConnection();
        connection.setConnectTimeout(8000);
        connection.setReadTimeout(8000);
        connection.setRequestProperty("User-Agent", "AirlineTycoonCE-Android");
        try (InputStream in = connection.getInputStream()) {
            XmlPullParser parser = Xml.newPullParser();
            parser.setInput(in, null);
            boolean inItem = false;
            for (int event = parser.getEventType(); event != XmlPullParser.END_DOCUMENT; event = parser.next()) {
                if (event == XmlPullParser.START_TAG) {
                    if ("item".equals(parser.getName())) {
                        inItem = true;
                    } else if (inItem && "title".equals(parser.getName())) {
                        String title = clean(parser.nextText(), german);
                        if (title.length() >= 12 && title.length() <= MAX_LENGTH && !result.contains(title)) {
                            result.add(title);
                            if (result.size() >= MAX_HEADLINES) {
                                break;
                            }
                        }
                    }
                } else if (event == XmlPullParser.END_TAG && "item".equals(parser.getName())) {
                    inItem = false;
                }
            }
        } finally {
            connection.disconnect();
        }
        return result;
    }

    /** The game's fonts only know ISO-8859-1, and "%" would be read as a placeholder. */
    static String clean(String text, boolean german) {
        StringBuilder out = new StringBuilder();
        for (int i = 0; i < text.length(); i++) {
            char c = text.charAt(i);
            switch (c) {
            case '“': case '”': case '„': case '«': case '»':
                out.append('"');
                break;
            case '‘': case '’': case '‚':
                out.append('\'');
                break;
            case '–': case '—':
                out.append('-');
                break;
            case '…':
                out.append("...");
                break;
            case ' ': case '\n': case '\r': case '\t':
                out.append(' ');
                break;
            case '%':
                out.append(german ? " Prozent" : " percent");
                break;
            default:
                if (c >= 0x20 && c <= 0xFF && !(c >= 0x7F && c <= 0x9F)) {
                    out.append(c);
                }
                break;
            }
        }
        return out.toString().replaceAll(" +", " ").trim();
    }
}
