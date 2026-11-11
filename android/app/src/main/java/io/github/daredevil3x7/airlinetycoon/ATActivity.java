package io.github.daredevil3x7.airlinetycoon;

import android.media.AudioManager;

import org.libsdl.app.SDLActivity;

/** The game itself: SDL's activity, loading the libraries built in app/jni. */
public class ATActivity extends SDLActivity {
    private AudioManager audioManager;

    /** Stops music and sounds while another app has the audio focus (phone call, other player). */
    private final AudioManager.OnAudioFocusChangeListener focusListener = new AudioManager.OnAudioFocusChangeListener() {
        @Override
        public void onAudioFocusChange(int change) {
            if (change == AudioManager.AUDIOFOCUS_LOSS || change == AudioManager.AUDIOFOCUS_LOSS_TRANSIENT) {
                setAudioFocusLost(true);
            } else if (change == AudioManager.AUDIOFOCUS_GAIN) {
                setAudioFocusLost(false);
            }
        }
    };

    private static native void nativeSetAudioFocusLost(boolean lost);

    private static void setAudioFocusLost(boolean lost) {
        try {
            nativeSetAudioFocusLost(lost);
        } catch (UnsatisfiedLinkError e) {
            // the game library is not loaded (yet)
        }
    }

    @Override
    protected String[] getLibraries() {
        return new String[] {"c++_shared", "SDL2", "SDL2_image", "SDL2_mixer", "SDL2_ttf", "main"};
    }

    /** Command line of the game: the settings made in the launcher. */
    @Override
    protected String[] getArguments() {
        java.util.ArrayList<String> arguments = new java.util.ArrayList<>();
        int language = ImportActivity.getLanguage(this);
        if (language >= 0) {
            arguments.add("/lang=" + language);
        }
        if (!ImportActivity.areExtrasEnabled(this)) {
            arguments.add("/noextras");
        }
        return arguments.toArray(new String[0]);
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (audioManager == null) {
            audioManager = (AudioManager) getSystemService(AUDIO_SERVICE);
        }
        if (audioManager != null) {
            audioManager.requestAudioFocus(focusListener, AudioManager.STREAM_MUSIC, AudioManager.AUDIOFOCUS_GAIN);
        }
        setAudioFocusLost(false);
    }

    @Override
    protected void onDestroy() {
        if (audioManager != null) {
            audioManager.abandonAudioFocus(focusListener);
        }
        super.onDestroy();
    }
}
