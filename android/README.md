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
| Swipe (move right away) | Hold the left button and move |
| Hold, then move if you like | Only moves the cursor: read flight orders and tooltips, scroll the airport on its arrows. Nothing is clicked. |
| Two-finger tap, Back button or the arrow button | Right click (leave room, close menu) |
| Tap twice on a room's exit strip | Leave the room (a single tap there does nothing) |
| In the laptop or at the globe: press an order or a flight, drag, lift | Moves it. An order that sticks to the cursor follows the finger and is put down where the finger is lifted. |
| Three-finger tap | Show / hide the keyboard (it opens by itself for savegame and player names) |
| Pinch with two fingers | Zoom in and out; drag with two fingers to move the zoomed picture |

On wide screens the picture is shown 20% wider than 4:3, and the bars beside it hold buttons:
menu (Esc), quick save, quick load, schedule and keyboard on the left; zoom, faster, slower and right click
on the right. Quick load needs a second press within three seconds. Both use their own
savegame slot, which the options menu does not list.

The schedule button works while the laptop or the globe is open: it puts the orders that were
accepted but not scheduled yet into the plans of the planes, using the planner of the computer
players. Planes that fly routes are not touched.

## Settings

Long-press the app icon and choose "Settings":

- Game language (follows the device language by default)
- Extra rules in the free game (on by default, see below)
- Export and import of savegames as a ZIP

The game is saved to the autosave slot whenever the app goes to the background, because
Android may end background apps at any time.

## Extra rules in the free game

These change the game itself and are therefore optional (`/extras` and `/noextras` on the
command line; on by default only on Android). They do not apply to missions and network games.

- **Two gates at the start:** every airline starts with two gates. The airport is then at its
  largest size (8 gates) from the first day, so there are no gate auctions and no expansions.
- **Events:** holidays (flight orders pay a quarter more), trade fairs (more and better paid
  orders to one city), kerosene price jumps in both directions and storms (many more
  breakdowns of planes without the best equipment). At most one event in six days. They follow
  from the start time of the game alone, so savegames are unaffected.
- **Newspapers:** announce each event one day ahead and report the kerosene price trend.
- **Sabotage by mood:** computer players sabotage the airline they like least, never a random
  one; the worse the relationship, the nastier the jobs. They also strike back at the human
  player once they dislike them enough, at every difficulty level.

- **Orders for older planes:** about 40 percent of the new orders fit the smallest plane of
  the fleet, so older planes still find work after bigger ones were bought.
- **Side jobs:** every five days somebody calls with a job for three days (carry passengers,
  complete flights or accept orders; the goal follows from the days before). A finished job
  pays a bonus and improves the image, a failed one costs nothing.
- **Statistics:** the own column shows all figures without an advisor; a total over figures
  that are hidden shows ??? instead of a wrong number.
- **Image:** every day the image follows the share of satisfied passengers of the day before:
  it rises by 1 or 2 points up to a ceiling (80 percent: 100, 90 percent: 300, 100 percent: 500)
  and falls by 2 or 4 points below 55 percent; the secretary reports it, and explains the image when asked at the agency.
- **Deals with rivals:** the conversation with a computer player (in the airport or by phone)
  has a new line "I have a deal to propose": a pact against sabotage (7 days), a price
  agreement (5 days, 10 percent more ticket income, 8 percent risk a day of a fine and an image
  loss), a joint kerosene purchase (15 percent back), passing an unscheduled order on for a
  commission or taking one over for a fee, and spreading rumours about another airline. Whether
  the rival agrees depends on how much they like the player.
- **More statistics:** the third page lists income per flight and per plane, passengers per
  flight, share of satisfied passengers, market share, finished side jobs and the bank balance
  of fixed deposit and term loan.
- **Morning meeting:** after his remarks about each airline the manager adds a briefing: who
  leads in company value and where the player stands, who carried the most passengers, who is
  angry with the player, the event of the day, the kerosene price, orders due today - and a
  remark of his own.
- **Bank:** at the credit desk a fixed deposit (7 days, 2 percent) and a term loan (20 days,
  3 percent, counts against the credit limit); at the stock desk short sales against the other
  airlines (settled after 5 days or on request, the value of the shares has to be in the account).
  These contracts are stored in `game<N>.fin` next to the savegame.
- **Stock market:** share prices react to the events; the newspapers report the biggest mover
  of the day and airlines that are being bought up (from one quarter of the shares on), and
  the advisor warns when a competitor holds 30 percent of the player's airline.

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
