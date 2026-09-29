#!/usr/bin/env python3
"""Minimal X11 input driver + screenshot helper for headless UI captures.

Uses libX11/libXtst through ctypes (no python-xlib needed) so it works in a bare
container with an Xvfb display. Screenshots go through ImageMagick `import`.

Usage as a library:
    from xdrive import XDrive
    x = XDrive(":99")
    x.key("Escape"); x.type_text("hello"); x.click(100, 200); x.shot("out.png")

Usage as a CLI:
    xdrive.py --display :99 key Return
    xdrive.py --display :99 type "some text"
    xdrive.py --display :99 click 640 400 [button]
    xdrive.py --display :99 move 640 400
    xdrive.py --display :99 shot out.png
    xdrive.py --display :99 sleep 1.5
Several commands can be chained with ';' inside one argument list:
    xdrive.py --display :99 key Escape ';' sleep 0.5 ';' shot a.png
"""
import ctypes
import ctypes.util
import os
import subprocess
import sys
import time

_KEYSYM_ALIASES = {
    "esc": "Escape", "enter": "Return", "ret": "Return", "tab": "Tab", "space": "space",
    "up": "Up", "down": "Down", "left": "Left", "right": "Right", "bs": "BackSpace",
    "pgup": "Prior", "pgdn": "Next", "home": "Home", "end": "End", "del": "Delete",
}

# Characters that need Shift on a US keymap (X11 keysym names for the shifted char).
_SHIFTED = {
    "~": "asciitilde", "!": "exclam", "@": "at", "#": "numbersign", "$": "dollar",
    "%": "percent", "^": "asciicircum", "&": "ampersand", "*": "asterisk",
    "(": "parenleft", ")": "parenright", "_": "underscore", "+": "plus",
    "{": "braceleft", "}": "braceright", "|": "bar", ":": "colon", '"': "quotedbl",
    "<": "less", ">": "greater", "?": "question",
}
_UNSHIFTED = {
    " ": "space", "-": "minus", "=": "equal", "[": "bracketleft", "]": "bracketright",
    "\\": "backslash", ";": "semicolon", "'": "apostrophe", ",": "comma", ".": "period",
    "/": "slash", "`": "grave", "\n": "Return", "\t": "Tab",
}


class XDrive:
    def __init__(self, display=None):
        display = display or os.environ.get("DISPLAY", ":99")
        self.display_name = display
        self.x11 = ctypes.CDLL(ctypes.util.find_library("X11") or "libX11.so.6")
        self.xtst = ctypes.CDLL(ctypes.util.find_library("Xtst") or "libXtst.so.6")
        self.x11.XOpenDisplay.restype = ctypes.c_void_p
        self.x11.XOpenDisplay.argtypes = [ctypes.c_char_p]
        self.x11.XStringToKeysym.restype = ctypes.c_ulong
        self.x11.XStringToKeysym.argtypes = [ctypes.c_char_p]
        self.x11.XKeysymToKeycode.restype = ctypes.c_ubyte
        self.x11.XKeysymToKeycode.argtypes = [ctypes.c_void_p, ctypes.c_ulong]
        self.x11.XFlush.argtypes = [ctypes.c_void_p]
        self.x11.XSync.argtypes = [ctypes.c_void_p, ctypes.c_int]
        self.xtst.XTestFakeKeyEvent.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.c_int,
                                                ctypes.c_ulong]
        self.xtst.XTestFakeButtonEvent.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.c_int,
                                                   ctypes.c_ulong]
        self.xtst.XTestFakeMotionEvent.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int,
                                                   ctypes.c_int, ctypes.c_ulong]
        self.dpy = self.x11.XOpenDisplay(display.encode())
        if not self.dpy:
            raise RuntimeError("cannot open display %s" % display)

    # --- keyboard -----------------------------------------------------------
    def _keycode(self, keysym_name):
        ks = self.x11.XStringToKeysym(keysym_name.encode())
        if ks == 0:
            raise ValueError("unknown keysym %r" % keysym_name)
        kc = self.x11.XKeysymToKeycode(self.dpy, ks)
        if kc == 0:
            raise ValueError("no keycode for keysym %r" % keysym_name)
        return kc

    def key(self, name, hold=0.04, shift=False, ctrl=False, alt=False):
        """Press and release one key. name is an X keysym ('Escape', 'a', 'Return')."""
        name = _KEYSYM_ALIASES.get(name.lower(), name) if len(name) > 1 else name
        mods = []
        if shift:
            mods.append(self._keycode("Shift_L"))
        if ctrl:
            mods.append(self._keycode("Control_L"))
        if alt:
            mods.append(self._keycode("Alt_L"))
        kc = self._keycode(name)
        for m in mods:
            self.xtst.XTestFakeKeyEvent(self.dpy, m, 1, 0)
        self.xtst.XTestFakeKeyEvent(self.dpy, kc, 1, 0)
        self.x11.XFlush(self.dpy)
        time.sleep(hold)
        self.xtst.XTestFakeKeyEvent(self.dpy, kc, 0, 0)
        for m in reversed(mods):
            self.xtst.XTestFakeKeyEvent(self.dpy, m, 0, 0)
        self.x11.XFlush(self.dpy)
        time.sleep(hold)

    def type_text(self, text, delay=0.05):
        for ch in text:
            if ch in _SHIFTED:
                self.key(_SHIFTED[ch], shift=True)
            elif ch in _UNSHIFTED:
                self.key(_UNSHIFTED[ch])
            elif ch.isalpha() and ch.isupper():
                self.key(ch.lower(), shift=True)
            elif ch.isalnum():
                self.key(ch)
            else:
                raise ValueError("cannot type %r" % ch)
            time.sleep(delay)

    # --- mouse --------------------------------------------------------------
    def move(self, x, y, settle=0.08):
        self.xtst.XTestFakeMotionEvent(self.dpy, -1, int(x), int(y), 0)
        self.x11.XFlush(self.dpy)
        self.last_pos = (int(x), int(y))
        time.sleep(settle)

    def nudge(self):
        """Move the pointer by one pixel and back: wakes an input-driven client so it renders
        a frame (the game loop blocks on input while nothing is open)."""
        x, y = getattr(self, "last_pos", (10, 10))
        self.move(x + 1, y, settle=0.05)
        self.move(x, y, settle=0.05)

    def click(self, x, y, button=1, hold=0.15, settle=0.15):
        """Software-renderer friendly click: move, wait a frame or two so the hover state is
        registered, press, hold, release."""
        self.move(x, y, settle=0.3)
        self.xtst.XTestFakeButtonEvent(self.dpy, button, 1, 0)
        self.x11.XFlush(self.dpy)
        time.sleep(hold)
        self.xtst.XTestFakeButtonEvent(self.dpy, button, 0, 0)
        self.x11.XFlush(self.dpy)
        time.sleep(settle)

    def drag(self, x0, y0, x1, y1, steps=12, button=1):
        self.move(x0, y0)
        self.xtst.XTestFakeButtonEvent(self.dpy, button, 1, 0)
        self.x11.XFlush(self.dpy)
        time.sleep(0.15)
        for i in range(1, steps + 1):
            t = i / steps
            self.move(x0 + (x1 - x0) * t, y0 + (y1 - y0) * t, settle=0.05)
        time.sleep(0.15)
        self.xtst.XTestFakeButtonEvent(self.dpy, button, 0, 0)
        self.x11.XFlush(self.dpy)
        time.sleep(0.2)

    def scroll(self, x, y, clicks=1):
        self.move(x, y)
        button = 4 if clicks > 0 else 5
        for _ in range(abs(clicks)):
            self.xtst.XTestFakeButtonEvent(self.dpy, button, 1, 0)
            self.xtst.XTestFakeButtonEvent(self.dpy, button, 0, 0)
            self.x11.XFlush(self.dpy)
            time.sleep(0.05)

    # --- capture ------------------------------------------------------------
    def shot(self, path, settle=0.4):
        time.sleep(settle)
        os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
        subprocess.run(["import", "-display", self.display_name, "-window", "root", path],
                       check=True)
        return path


def _run_cli(argv):
    display = None
    if argv and argv[0] == "--display":
        display, argv = argv[1], argv[2:]
    x = XDrive(display)
    cmds, cur = [], []
    for a in argv:
        if a == ";":
            cmds.append(cur)
            cur = []
        else:
            cur.append(a)
    if cur:
        cmds.append(cur)
    for c in cmds:
        if not c:
            continue
        op = c[0]
        if op == "key":
            for k in c[1:]:
                x.key(k)
        elif op == "type":
            x.type_text(" ".join(c[1:]))
        elif op == "click":
            x.click(int(c[1]), int(c[2]), int(c[3]) if len(c) > 3 else 1)
        elif op == "move":
            x.move(int(c[1]), int(c[2]))
        elif op == "drag":
            x.drag(int(c[1]), int(c[2]), int(c[3]), int(c[4]))
        elif op == "scroll":
            x.scroll(int(c[1]), int(c[2]), int(c[3]) if len(c) > 3 else 1)
        elif op == "shot":
            x.shot(c[1])
        elif op == "sleep":
            time.sleep(float(c[1]))
        else:
            raise SystemExit("unknown command %r" % op)


if __name__ == "__main__":
    _run_cli(sys.argv[1:])
