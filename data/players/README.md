# Players

Put the known players of your group here, so they can pick themselves in the menu.

`players.txt` lists one player per line: the name, then optionally a semicolon and the
file name of a face image in this folder. Lines starting with `#` are ignored.

```
Anna;anna.png
Bruno;bruno.jpg
Chloé
```

Face images can be PNG, JPEG or BMP. They are cut to a square from the centre and wrapped
around the blob, so a photo cropped closely around the face works best. Transparent parts
of a PNG show the player's colour instead.

`tools/faces/prepare_faces.py` makes these from ordinary photos: it finds the face (also in
tilted or sideways photos), turns it upright, crops the head to fit the blob, cuts out
face and hair and writes the PNG plus the line in `players.txt`:

```
uv venv -p 3.12 /tmp/faces-venv && VIRTUAL_ENV=/tmp/faces-venv uv pip install -r tools/faces/requirements.txt
/tmp/faces-venv/bin/python tools/faces/prepare_faces.py ~/photos --names names.txt --preview sheet.jpg
```

`names.txt` maps photos to names, one `photo;Name` per line, where `photo` is the start of
the photo's file name. Look at `sheet.jpg` to check the results.

Everything in this folder except this README is ignored by git, so photos of your friends
don't end up in a public repository by accident.
