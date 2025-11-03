# XKB Keyboard Layout Generator

This program takes a `.kb` file (described below) and translates it to an XKB
keyboard; I use it to simplify editing my keyboard layout.

## KB file format
KB files are text files that consist of a series of records. Each record has the
form:
```ini
# this is a comment
key = [ a, b, c, d, "comma", '"', ... ]
```

The keys are symbolic XKB names (e.g. `<TLDE>`); if you’re unsure what name a key
on your keyboard has, run `xev | grep keycode` and press the key whose name you 
want to know, e.g. the `1` key. The output will be something like this:
```console
state 0x10, keycode 10 (keysym 0x31, 1), same_screen YES
``` 
This means that the keycode that corresponds to the `1` key is `10`; armed with this
knowledge, open `/usr/share/X11/xkb/keycodes/evdev`, where you’ll see something like
this
```ini
<TLDE> = 49; 
<AE01> = 10; 
<AE02> = 11; 
<AE03> = 12; 
<AE04> = 13; 
<AE05> = 14; 
...
```
This tells us that the name corresponding of the key with keycode `10` (i.e. the `1`
key) is `<AE01>`, which means that we can assign characters to the `1` as follows:
```ini
<AE01> = [ 1 ! ¡ ₁ ]
```
