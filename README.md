# AlsaMidiRemapper
A utility to filter or change MIDI notes from an input to an output using ALSA

## Building
Something like:
```bash
$ git clone --depth 1 https://github.com/danieloneill/AlsaMidiRemapper.git
$ cd AlsaMidiRemapper
$ mkdir build
$ cd build
$ qmake6 ..
$ make -j32
$ ./AlsaMidiRemapper
```

## Using
Select your SOURCE MIDI device in the *From* combo box:

![from](https://github.com/user-attachments/assets/1b9e6c6b-0982-4450-b73d-2d596f64293b)



Then your DESTINATION MIDI device on the *To* combo box:

![to](https://github.com/user-attachments/assets/c9cd689c-5462-4ca8-9ac0-0ba121776798)



Toggle the *Connect* checkbox:

![connect](https://github.com/user-attachments/assets/04f32337-1594-4650-a899-987fed5fd25f)

Once connected, AlsaMidiRemapper will subscribe to all MIDI events from your *From*.

By default, all events are then proxied to the *To* MIDI device unmodified and unfiltered.

You can change the note ID transmitted for each note ID received by changing the number:

![controls](https://github.com/user-attachments/assets/10018e80-a892-4caa-934a-04177a59bb31)


Toggling the switch for a note will filter it out entirely.

To add notes to the list to filter/change, transmit the note somehow. (Hit a key on the keyboard, hit the drum/cymbal, etc.)

An indicator will illuminate on a note's row when it is triggered (unless it is disabled).
