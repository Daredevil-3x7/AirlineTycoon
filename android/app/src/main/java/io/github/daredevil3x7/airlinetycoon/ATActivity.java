package io.github.daredevil3x7.airlinetycoon;

import org.libsdl.app.SDLActivity;

/** The game itself: SDL's activity, loading the libraries built in app/jni. */
public class ATActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] {"c++_shared", "SDL2", "SDL2_image", "SDL2_mixer", "SDL2_ttf", "main"};
    }

    /** Command line of the game: the language chosen in the launcher, if any. */
    @Override
    protected String[] getArguments() {
        int language = ImportActivity.getLanguage(this);
        if (language >= 0) {
            return new String[] {"/lang=" + language};
        }
        return new String[0];
    }
}
