# Airline Tycoon Deluxe on Android (fan build)

Experimental Android port of the community version. Non-commercial, like the rest of this
repository (see `License.txt`). The APK contains **no game data**: every player imports the
files of their own copy of Airline Tycoon Deluxe (GOG or Steam).

## Installing

1. Download the APK from the latest "Android preview" release and install it
   (allow installing from this source when Android asks).
2. On a computer, pack the game folder (`Airline Tycoon Deluxe`) into a ZIP and copy it to
   the phone.
3. Start the app and select the ZIP. The game folders (`data`, `gli`, `intro`, `misc`, `room`,
   `sound`, `video`, `voice`) are unpacked into `Android/data/io.github.daredevil3x7.airlinetycoon/files`.
   Copying these folders there by hand works as well. About 400 MB are needed.

Savegames, settings (`AT.json`) and the log (`debug.txt`) are stored in the same folder.

## Controls

| Touch | Game |
| --- | --- |
| Tap | Left click |
| Double tap | Double click |
| Drag | Hold the left button and move |
| Long press, two-finger tap or Back button | Right click (leave room, close menu) |
| Three-finger tap | Show / hide the keyboard (it opens by itself for savegame and player names) |
| Pinch with two fingers | Zoom in and out; drag with two fingers to move the zoomed picture |

On wide screens the picture is shown 20% wider than 4:3, and the bars beside it hold buttons:
menu (Esc), quick save, quick load and keyboard on the left; zoom, faster, slower and right click
on the right. Quick load needs a second press within three seconds. Both use their own
savegame slot, which the options menu does not list.

## Settings

Long-press the app icon and choose "Settings":

- Game language (follows the device language by default)
- Real headlines in the kiosk: titles from the RSS feed of tagesschau.de (German) or BBC News
  (other languages) are added to free lines of the newspapers. This is the only use of the
  network and can be switched off.
- Export and import of savegames as a ZIP

The game is saved to the autosave slot whenever the app goes to the background, because
Android may end background apps at any time.

## Limitations

- Music: only the OGG tracks play. MIDI is not available on Android.
- Multiplayer is untested.
- Only 64-bit ARM devices (arm64-v8a), Android 7.0 or newer.

## Building

The workflow `.github/workflows/android.yml` does all of this:

1. Build the Linux version and copy `patch/*.res` from its output folder to
   `android/app/src/main/assets/patch/` (the game compiles these files itself, which cannot
   be done when cross-compiling).
2. Clone SDL2, SDL2_image, SDL2_mixer and SDL2_ttf (with `external/freetype`) into
   `android/app/jni/SDL`, `SDL_image`, `SDL_mixer` and `SDL_ttf`.
3. Run `gradle assembleRelease` in `android/` (JDK 17, Gradle 8.9, Android SDK with NDK).

All builds are signed with the key in `app/debug.keystore`. It is public on purpose: this is
a sideloading build, and a fixed key lets every build update the previous one without
losing the imported game files.

## What differs from the desktop version

- `src/Misc.cpp`: the game folder is the app's external files directory.
- `src/Takeoff.cpp`: the game runs on a thread with a 16 MB stack.
- `src/GameFrame.cpp`: touch gestures are translated to mouse events.
- `app/jni/CMakeLists.txt`: compiles with `-fsigned-char`, as `char` is unsigned on ARM Android.
