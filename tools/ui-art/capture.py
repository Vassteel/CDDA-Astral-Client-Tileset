#!/usr/bin/env python3
"""Drive the tiles client on a private Xvfb display and capture native screenshots.

    tools/ui-art/capture.py --size 1280x800 --out artifacts/ui-art-overhaul/m0 \
        [--binary build/cataclysm-tiles] [--profile DIR] [--display :99] [--scenario NAME]
        [--font 16] [--keep]

Scenarios (each is a list of steps; every step is a named screenshot or an input):
  menu       main menu, Settings submenu, showcase (needs CDDA_UI_SHOWCASE build)
  showcase   only the component showcase (three pages, wide + narrow)
  newgame    Play Now! → in-game HUD → equipment → character sheet → crafting → …
  load       load the first character in the first world, then the in-game set
  chargen    New Game → Custom Character: every creator tab, then cancel back to the menu
  world      World → Create World: basic confirm and the advanced (mods / options) tabs

The profile is disposable; the user's installation is never touched. Output files are
`<out>/<name>-<WxH>.png` plus `index.md` listing what each capture shows and how it was
obtained (build id, profile, display, scenario).
"""
import argparse
import json
import os
import shutil
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from xdrive import XDrive  # noqa: E402

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
LOGS = os.path.join(ROOT, "artifacts", "ui-art-overhaul", "logs")


def sh(cmd, **kw):
    return subprocess.run(cmd, shell=True, check=False, capture_output=True, text=True, **kw)


class Client:
    def __init__(self, size, display, profile, binary, env=None, args="", root=None):
        self.size, self.display, self.profile, self.binary = size, display, profile, binary
        self.env = env or {}
        self.args = args
        self.root = root or ROOT
        self.pid = None

    def start(self):
        env = " ".join("%s=%s" % (k, v) for k, v in self.env.items())
        r = sh('%s %s -s %s -d %s -p "%s" -b "%s" -r "%s" -- %s' % (
            env, os.path.join(ROOT, "tools/ui-art/run_client.sh"), self.size, self.display,
            self.profile, self.binary, self.root, self.args))
        print(r.stdout.strip())
        for tok in r.stdout.split():
            if tok.startswith("pid="):
                self.pid = int(tok[4:])
        return self

    def alive(self):
        return self.pid is not None and os.path.exists("/proc/%d" % self.pid)

    def stop(self):
        sh('%s -d %s' % (os.path.join(ROOT, "tools/ui-art/stop_client.sh"), self.display))


class Probe:
    """Reader for the client's widget probe file (CDDA_UI_PROBE)."""

    def __init__(self, path):
        self.path = path
        self.data = {"frame": 0, "widgets": []}
        self.frame_dt = 0.1
        self.nudge = None  # callable that wakes the client (see XDrive.nudge)

    def _load(self):
        for _ in range(20):
            try:
                with open(self.path) as f:
                    data = json.load(f)
                if data.get("frame", 0) != self.data.get("frame", 0):
                    self.last_change = time.time()
                self.data = data
                return self.data
            except (OSError, ValueError):
                time.sleep(0.1)
        return self.data

    def rendering(self, within=3.0):
        """True when the client rendered an ImGui frame in the last `within` seconds."""
        self._load()
        return time.time() - getattr(self, "last_change", 0) < within

    def loading(self):
        """True while the loading screen is on (a probe-recorded "Loading" window in a frame
        rendered recently)."""
        return self.rendering() and self.first("window", "Loading", True) is not None

    def read(self, settle=0.5, frames=4, timeout=8.0):
        """Wait `settle` seconds, then until the client has rendered at least `frames` more
        frames than the last read (so the layout reflects the input just sent; the software
        renderer at 4K runs at a few frames per second). ImGui trickles a move + press + release
        over three frames and buttons fire on release, so four frames covers one click."""
        time.sleep(settle)
        last = self.data.get("frame", 0)
        t0 = time.time()
        nudged = 0
        while True:
            self._load()
            if self.data.get("frame", 0) >= last + frames or time.time() - t0 > timeout:
                return self.data
            # The game loop blocks on input while no window is open, so the probe file goes
            # stale; a one-pixel mouse move makes it render again.
            if self.nudge and time.time() - t0 > 0.6 * (nudged + 1):
                self.nudge()
                nudged += 1
            time.sleep(0.1)

    def frame_time(self, sample=3.0):
        """Measure the client's frame time (s) from the probe's frame counter."""
        f0 = self._load().get("frame", 0)
        t0 = time.time()
        time.sleep(sample)
        f1 = self._load().get("frame", 0)
        dt = time.time() - t0
        self.frame_dt = dt / max(1, f1 - f0)
        return self.frame_dt

    def widgets(self, kind=None, label=None, exact=False, visible_only=True):
        out = []
        for w in self.data.get("widgets", []):
            if visible_only and not w.get("visible", True):
                continue
            if kind:
                # exact kind, or a prefix when it ends with '*' ("row*" = row, row_selected…)
                if kind.endswith("*"):
                    if not w["kind"].startswith(kind[:-1]):
                        continue
                elif w["kind"] != kind:
                    continue
            if label is not None:
                if exact and w["label"] != label:
                    continue
                if not exact and label.lower() not in w["label"].lower():
                    continue
            out.append(w)
        return out

    def first(self, kind=None, label=None, exact=False):
        ws = self.widgets(kind, label, exact)
        return ws[0] if ws else None

    @staticmethod
    def center(w):
        return ((w["min"][0] + w["max"][0]) // 2, (w["min"][1] + w["max"][1]) // 2)


def wait_for(probe, pred, timeout=15.0):
    """Poll the probe until pred(probe) holds. The timeout is stretched with the client's
    frame time so slow (4K software) renders get the same number of frames to react."""
    timeout *= max(1.0, probe.frame_dt / 0.1)
    t0 = time.time()
    while time.time() - t0 < timeout:
        probe.read(0.3)
        if pred(probe):
            return True
    return False


def hud_visible(p):
    return p.first("toolbar", "Inv") is not None


def wait_loaded(x, probe, max_wait=1200):
    """Wait for world generation / load: done when the HUD toolbar is in the probe, or when
    the screen has been still for a while (opening dialogue, popup, debug prompt)."""
    t0 = time.time()
    tmp1, tmp2 = "/tmp/cap_l1.png", "/tmp/cap_l2.png"
    x.shot(tmp1, settle=0)
    while time.time() - t0 < max_wait:
        probe.read(0.5, frames=1, timeout=2)
        if hud_visible(probe):
            return True
        time.sleep(4)
        x.shot(tmp2, settle=0)
        if open(tmp1, "rb").read() == open(tmp2, "rb").read() and not probe.loading():
            return True
        os.replace(tmp2, tmp1)
    return False


def ensure_hud(x, probe, shot=None, tries=10):
    """After a new game starts, get past the scenario's opening dialogue (if any) and any
    debug prompts until the in-game HUD (mouse toolbar) is on screen."""
    if not os.path.exists(probe.path):
        # No widget probe (baseline build): only the debug prompts can be detected. Return is
        # harmless on the HUD, picks the highlighted response in an opening dialogue and
        # confirms the "Achievement completed" dialog, so press it a few times.
        dismiss_debug_prompts(x)
        wait_stable(x, seconds=3, max_wait=60)
        if shot:
            shot("start-screen")
        for _ in range(4):
            x.key("Return")
            time.sleep(1.5)
            dismiss_debug_prompts(x)
            wait_stable(x, seconds=2, max_wait=60)
        return False
    for i in range(tries):
        dismiss_debug_prompts(x)
        if wait_for(probe, hud_visible, 3):
            return True
        if i == 0 and shot:
            shot("start-dialogue")
        # Opening dialogue: pick the first response (the conversation ends by itself);
        # "Achievement completed" / any-key popups: Return.
        x.key("a" if i % 2 == 0 else "Return")
        time.sleep(1.5)
    return wait_for(probe, hud_visible, 10)


def wait_stable(x, seconds=2.0, interval=0.5, max_wait=120):
    """Wait until two consecutive root screenshots are identical (loading done)."""
    tmp1 = "/tmp/cap_a.png"
    tmp2 = "/tmp/cap_b.png"
    t0 = time.time()
    x.shot(tmp1, settle=0)
    while time.time() - t0 < max_wait:
        time.sleep(interval)
        x.shot(tmp2, settle=0)
        if open(tmp1, "rb").read() == open(tmp2, "rb").read():
            time.sleep(seconds)
            x.shot(tmp2, settle=0)
            if open(tmp1, "rb").read() == open(tmp2, "rb").read():
                return True
        os.replace(tmp2, tmp1)
    return False


def is_debug_prompt(path):
    """True when the native debugmsg prompt (red report text on a dark, otherwise empty
    background, top-left) is on screen."""
    try:
        from PIL import Image
        im = Image.open(path).convert("RGB")
        px = im.load()
        reds = 0
        dark = 0
        total = 0
        # Header line "An error has occurred!" + the DEBUG line (font 16..32 → y 28..170).
        for x in range(8, 1100, 2):
            for y in range(24, 170, 2):
                r, g, b = px[x, y]
                total += 1
                if r >= 245 and 135 <= g <= 165 and 135 <= b <= 165:
                    reds += 1
                elif r < 50 and g < 50 and b < 50:
                    dark += 1
        return reds > 60 and dark > total * 0.85
    except Exception:
        return False


def dismiss_debug_prompts(x, limit=6):
    """Press 'i' (ignore this message in future) while the debugmsg prompt is showing."""
    for _ in range(limit):
        x.shot("/tmp/cap_dbg.png", settle=0.3)
        if not is_debug_prompt("/tmp/cap_dbg.png"):
            return
        print("  debug prompt on screen; ignoring it")
        x.key("i")
        time.sleep(2.5)


def set_options(profile, values):
    """Override options in the profile's options.json (creating entries as needed)."""
    path = os.path.join(profile, "config", "options.json")
    with open(path) as f:
        opts = json.load(f)
    seen = set()
    for entry in opts:
        if entry.get("name") in values:
            entry["value"] = values[entry["name"]]
            seen.add(entry["name"])
    for name, value in values.items():
        if name not in seen:
            opts.append({"info": "", "default": "", "name": name, "value": value})
    with open(path, "w") as f:
        json.dump(opts, f, indent=2)


def prepare_profile(profile, font, base=None):
    os.makedirs(os.path.join(profile, "config"), exist_ok=True)
    if base and os.path.isdir(base) and not os.listdir(profile):
        shutil.copytree(base, profile, dirs_exist_ok=True)
    opts = os.path.join(profile, "config", "options.json")
    if not os.path.exists(opts):
        # Minimal options: windowed at the Xvfb size, software-friendly settings.
        w, h = [int(v) for v in os.path.basename(profile).rsplit("-", 1)[-1].split("x")] if "x" in os.path.basename(profile) else (1280, 800)
        # The GUI (ImGui) font follows FONT_HEIGHT. Measured terminal cell sizes on this
        # renderer: 16 -> 8x16, 24 -> 18x33.4 (Terminus bitmap strike selection).
        cell_w, cell_h = font / 2.0, float(font)
        term_x, term_y = max(80, int(w // cell_w)), max(24, int(h // cell_h))
        with open(opts, "w") as f:
            f.write("""[
  {"info":"","default":"","name":"FULLSCREEN","value":"no"},
  {"info":"","default":"","name":"FONT_HEIGHT","value":"%d"},
  {"info":"","default":"","name":"FONT_SIZE","value":"%d"},
  {"info":"","default":"","name":"FONT_WIDTH","value":"%d"},
  {"info":"","default":"","name":"TERMINAL_X","value":"%d"},
  {"info":"","default":"","name":"TERMINAL_Y","value":"%d"},
  {"info":"","default":"","name":"TILES","value":"UltimateCataclysm"},
  {"info":"","default":"","name":"PIXEL_MINIMAP","value":"false"},
  {"info":"","default":"","name":"USE_TILES","value":"true"},
  {"info":"","default":"","name":"MOUSE_TOOLBAR","value":"true"},
  {"info":"","default":"","name":"RPG_EQUIPMENT_UI","value":"true"},
  {"info":"","default":"","name":"SHOW_MOUSE","value":"show"},
  {"info":"","default":"","name":"AUTOSAVE","value":"false"},
  {"info":"","default":"","name":"QUERY_DISASSEMBLE","value":"false"},
  {"info":"","default":"","name":"PROMPT_ON_CHARACTER_STAT_CHANGE","value":"false"},
  {"info":"","default":"","name":"MUSIC_VOLUME","value":"0"},
  {"info":"","default":"","name":"SOUND_EFFECT_VOLUME","value":"0"}
]
""" % (font, font, font // 2, term_x, term_y))


def font_size_dims(font):
    return font, font


def run_scenario(name, x, cli, out, tag, opts):
    shots = []

    def shot(label, settle=0.8):
        path = os.path.join(out, "%s-%s.png" % (label, tag))
        dismiss_debug_prompts(x)
        x.shot(path, settle=settle)
        shots.append((label, path))
        print("  captured", path)

    def key(*ks, wait=0.6):
        for k in ks:
            x.key(k)
            time.sleep(wait)

    print("waiting for the main menu…")
    wait_stable(x, seconds=3, max_wait=240)
    if name in ("menu", "menus2", "menus3", "wgoptions", "showcase", "newgame", "load", "chargen", "world"):
        if "CDDA_UI_SHOWCASE" in cli.env:
            # the showcase opens itself at startup
            shot("showcase-components")
            w, h = [int(v) for v in cli.size.split("x")]
            # Tab strip sits just under the title bar of the centred window.
            win_w = min(w * 0.94, 1500 * max(1.0, opts.font / 16.0))
            win_h = min(h * 0.94, 1000 * max(1.0, opts.font / 16.0))
            left, top = (w - win_w) / 2, (h - win_h) / 2
            x.click(int(left + 170 * opts.font / 16.0), int(top + 82 * opts.font / 16.0))
            time.sleep(0.5)
            shot("showcase-rows")
            x.click(int(left + 320 * opts.font / 16.0), int(top + 82 * opts.font / 16.0))
            time.sleep(0.5)
            shot("showcase-dialogs")
            key("Escape")
            wait_stable(x, seconds=1, max_wait=30)
        shot("main-menu")
        if name == "showcase":
            return shots
        key("t")  # Settings category (Escape at the top level would prompt to quit)
        shot("main-menu-settings")
        if name == "wgoptions":
            pr = opts.probe
            def click_label(kind, label, exact=True, settle=3.0):
                pr.read(0.5)
                wdg = pr.first(kind, label, exact)
                if wdg:
                    hold = max(0.3, 1.5 * getattr(pr, "frame_dt", 0.1))
                    x.click(*Probe.center(wdg), hold=hold, settle=hold)
                    time.sleep(settle)
                return wdg is not None
            key("Escape"); time.sleep(1)
            click_label("toolbar*", "World")
            if click_label("button*", "Create world…", settle=5.0):
                click_label("tab*", "World options")
                wait_stable(x, seconds=1.5, max_wait=40)
                shot("create-world-options")
                x.scroll(3, 8) if hasattr(x, "scroll") else None
            return shots
        if name == "menus3":
            # Settings → Options / Autopickup / Safemode / Colors, with slow-renderer waits.
            pr = opts.probe
            def click_label(kind, label, exact=True, settle=3.0):
                pr.read(0.5)
                wdg = pr.first(kind, label, exact)
                print("click", kind, label, "->", "hit" if wdg else "MISS")
                if wdg:
                    hold = max(0.3, 1.5 * getattr(pr, "frame_dt", 0.1))
                    x.click(*Probe.center(wdg), hold=hold, settle=hold)
                    time.sleep(settle)
                return wdg is not None
            for row, title in (("Options", "Settings"), ("Autopickup", "Auto pickup manager"),
                               ("Safemode", "Safe mode manager"), ("Colors", "Colors")):
                if click_label("row*", row):
                    click_label("button*", "Open", settle=6.0)
                    wait_stable(x, seconds=1.5, max_wait=40)
                    shot("dlg-" + row.lower())
                    if row == "Options":
                        for tab in ("Interface", "Graphics", "World Defaults", "Debug"):
                            if click_label("tab*", tab, settle=2.0):
                                shot("dlg-options-" + tab.lower().replace(" ", "-"))
                    click_label("button*", "Cancel", settle=4.0)
            return shots
        if name == "menus2":
            # Unified main-menu dialogs: New game / Load / Worlds / Settings, Options, world creator.
            pr = opts.probe
            def click_label(kind, label, exact=True, settle=1.0, expect=None):
                pr.read(0.5)
                wdg = pr.first(kind, label, exact)
                print("click", kind, label, "->", "hit" if wdg else "MISS",
                      "" if wdg else [w["label"] for w in pr.widgets()][:12])
                if wdg:
                    # Slow renderers: hold the press across at least one frame boundary,
                    # and retry once when the expected window did not appear (the first
                    # click after a screen closes can land before the hover is registered).
                    hold = max(0.2, 1.5 * getattr(pr, "frame_dt", 0.1))
                    for attempt in range(2):
                        x.click(*Probe.center(wdg), hold=hold, settle=hold)
                        time.sleep(max(settle, 2 * hold))
                        if not expect:
                            break
                        pr.read(0.5)
                        if pr.first("window", expect, True):
                            break
                return wdg is not None
            key("Escape")  # close the Settings dialog
            time.sleep(0.8)
            click_label("toolbar*", "New Game", expect="New game"); shot("dlg-newgame")
            click_label("toolbar*", "Load", expect="Load game"); shot("dlg-load")
            click_label("toolbar*", "World", expect="Worlds"); shot("dlg-worlds")
            if click_label("button*", "Create world…"):
                wait_stable(x, seconds=1.5, max_wait=40); shot("dlg-create-world-basics")
                click_label("tab*", "Mods", exact=False); shot("dlg-create-world-mods")
                click_label("tab*", "World options"); shot("dlg-create-world-options")
                click_label("button*", "Cancel"); time.sleep(0.8)
                click_label("button*", "[Y]es", exact=False); time.sleep(1.0)
            click_label("toolbar*", "Settings", expect="Settings"); shot("dlg-settings")
            if click_label("row*", "Options"):
                click_label("button*", "Open")
                wait_stable(x, seconds=1.5, max_wait=40); shot("dlg-options-general")
                for tab in ("Interface", "Graphics", "World Defaults", "Debug"):
                    if click_label("tab*", tab):
                        shot("dlg-options-" + tab.lower().replace(" ", "-"))
                click_label("button*", "Cancel"); time.sleep(1.0)
            if click_label("row*", "Autopickup", exact=False):
                click_label("button*", "Open"); wait_stable(x, seconds=1.5, max_wait=40); shot("dlg-autopickup")
                click_label("button*", "Cancel"); time.sleep(1.0)
            if click_label("row*", "Safemode", exact=False):
                click_label("button*", "Open"); wait_stable(x, seconds=1.5, max_wait=40); shot("dlg-safemode")
                click_label("button*", "Cancel"); time.sleep(1.0)
            if click_label("row*", "Colors", exact=False):
                click_label("button*", "Open"); wait_stable(x, seconds=1.5, max_wait=40); shot("dlg-colors")
                click_label("button*", "Cancel"); time.sleep(1.0)
            return shots
        if name == "menu":
            key("a")
            shot("main-menu-load")
            key("w")
            shot("main-menu-world")
            key("c")
            shot("credits")
            key("m")
            shot("motd")
            return shots
    if name == "chargen":
        key("n")
        key("Return")  # first drawer entry: Custom Character ('u' would hit the Tutorial hotkey)
        wait_stable(x, seconds=2, max_wait=120)
        dismiss_debug_prompts(x)
        # With no world yet, Create World comes first: Finish it with the defaults.
        if not wait_for(opts.probe, lambda p: p.first("window", "Character creator", True) is not None, 3):
            shot("chargen-world-first")
            key("f")
            time.sleep(1.0)
            shot("chargen-world-finish-query")
            x.key("y", shift=True)  # "Are you SURE you're finished?" (case sensitive)
            # data loads for the new world before the creator appears
            if os.path.exists(opts.probe.path):
                wait_for(opts.probe, lambda p: p.first("window", "Character creator", True) is not None, 400)
            else:
                wait_stable(x, seconds=6, max_wait=400)
            dismiss_debug_prompts(x)
        for tab in ("scenario", "profession", "background", "stats", "traits", "skills", "equipment", "summary"):
            shot("chargen-" + tab)
            key("Tab")
            time.sleep(0.8)
        key("Escape")  # "Return to main menu?" query
        time.sleep(0.8)
        shot("chargen-cancel-query")
        x.key("y", shift=True)
        wait_stable(x, seconds=1, max_wait=30)
        return shots
    if name == "world":
        key("w")
        time.sleep(0.6)
        key("Return")  # Create World (first entry)
        wait_stable(x, seconds=1.5, max_wait=60)
        dismiss_debug_prompts(x)
        shot("worldgen-basic")
        key("m")  # PICK_MODS
        wait_stable(x, seconds=1.5, max_wait=60)
        shot("worldgen-mods")
        key("Escape")
        time.sleep(0.8)
        key("s")  # ADVANCED_SETTINGS (world options tab)
        wait_stable(x, seconds=1.5, max_wait=60)
        shot("worldgen-options")
        key("Tab")
        time.sleep(0.8)
        shot("worldgen-options-tab2")
        for _ in range(3):
            key("Escape")
            time.sleep(0.8)
            dismiss_debug_prompts(x)
        shot("worldgen-after-escape")
        return shots
    if name == "newgame":
        key("n")
        shot("main-menu-newgame")
        key("o")  # Play Now!
        time.sleep(2)
        shot("newgame-after-playnow", settle=1.5)
        print("waiting for world generation / load…")
        wait_loaded(x, opts.probe)
        ensure_hud(x, opts.probe, shot)
        shot("hud")
    if name == "load":
        key("l")
        time.sleep(1)
        key("Return")
        time.sleep(1)
        shot("load-character-list")
        key("Return")
        time.sleep(1)
        key("Return")
        print("waiting for load…")
        wait_loaded(x, opts.probe)
        ensure_hud(x, opts.probe, shot)
        shot("hud")
    if name == "fixcheck":
        # Targeted re-check after playtest fixes: HUD, equipment grid, body tab, AIM.
        key("n")
        key("o")
        time.sleep(2)
        wait_loaded(x, opts.probe)
        ensure_hud(x, opts.probe, shot)
        shot("hud")
        key("i")
        wait_stable(x, seconds=1, max_wait=30)
        shot("equipment")
        key("Escape")
        time.sleep(1)
        x.type_text("@")
        wait_stable(x, seconds=1, max_wait=40)
        body = None
        if opts.probe:
            opts.probe.read(0.5)
            body = opts.probe.first("tab*", "Body", True)
        if body:
            x.click(*Probe.center(body))
        else:
            key("Tab", "Tab", "Tab")
        time.sleep(3)  # slow renderers: give the tab switch a few frames
        wait_stable(x, seconds=1, max_wait=20)
        shot("character-body")
        key("Escape")
        time.sleep(1.2)
        x.type_text("/")
        wait_stable(x, seconds=1, max_wait=40)
        shot("aim")
        key("Escape")
        time.sleep(1)
        return shots
    if name in ("newgame", "load"):
        key("i")
        wait_stable(x, seconds=1, max_wait=30)
        shot("equipment")
        key("Down", "Down", "Right")
        shot("equipment-expanded")
        key("Escape", "Escape")  # collapse group, close window
        time.sleep(1)
        def screen(k, label, shift=False):
            if shift:
                x.type_text(k)
            else:
                x.key(k)
            wait_stable(x, seconds=1, max_wait=40)
            shot(label)
            key("Escape")
            time.sleep(1.2)
        screen("@", "character", True)
        # Body tab of the character sheet (three tabs to the right of Stats).
        x.type_text("@")
        wait_stable(x, seconds=1, max_wait=40)
        key("Right", "Right", "Right")
        wait_stable(x, seconds=1, max_wait=20)
        shot("character-body")
        key("Escape")
        time.sleep(1.2)
        screen("&", "crafting", True)
        screen("*", "construction", True)
        screen("/", "aim", True)
        screen("m", "overmap")
        screen("M", "missions", True)
        screen("E", "consume", True)
        screen("P", "messages", True)
        key("Escape")  # in-game escape menu (uilist)
        time.sleep(1)
        shot("escape-menu")
        key("Escape")
        time.sleep(1)
        shot("hud-final")
    return shots


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--size", default="1280x800")
    ap.add_argument("--out", required=True)
    ap.add_argument("--binary", default=os.path.join(ROOT, "build", "src", "cataclysm-tiles"))
    ap.add_argument("--root", default=None, help="game root (cwd for data/ and gfx/); default: this repo")
    ap.add_argument("--profile", default=None)
    ap.add_argument("--set", action="append", default=[], metavar="NAME=VALUE",
                    help="override an option in the profile (e.g. PIXEL_MINIMAP=true), repeatable")
    ap.add_argument("--base-profile", default=None, help="copy this profile first (saves)")
    ap.add_argument("--display", default=":99")
    ap.add_argument("--scenario", default="menu")
    ap.add_argument("--font", type=int, default=16)
    ap.add_argument("--tag", default=None)
    ap.add_argument("--keep", action="store_true")
    ap.add_argument("--env", action="append", default=[])
    a = ap.parse_args()
    out = os.path.abspath(a.out)
    os.makedirs(out, exist_ok=True)
    tag = a.tag or a.size
    profile = os.path.abspath(a.profile or os.path.join(ROOT, "artifacts", "ui-art-overhaul", "profiles",
                                                         "%s-%s" % (a.scenario, a.size)))
    prepare_profile(profile, a.font, a.base_profile)
    if a.set:
        set_options(profile, dict(kv.split("=", 1) for kv in a.set))
    env = dict(kv.split("=", 1) for kv in a.env)
    probe_path = os.path.join(profile, "config", "ui-probe.json")
    env.setdefault("CDDA_UI_PROBE", probe_path)
    cli = Client(a.size, a.display, profile, a.binary, env, root=a.root).start()
    time.sleep(3)
    if not cli.alive():
        print("client did not start; see", LOGS)
        return 1
    x = XDrive(a.display)
    a.probe = Probe(probe_path)
    a.probe.nudge = x.nudge
    build_id = sh("cd %s && git rev-parse --short HEAD" % (a.root or ROOT)).stdout.strip()
    try:
        shots = run_scenario(a.scenario, x, cli, out, tag, a)
    finally:
        if not a.keep:
            cli.stop()
    with open(os.path.join(out, "index.md"), "a") as f:
        f.write("\n## %s @ %s (font %d) — build %s, scenario %s, display %s, profile %s\n\n" % (
            time.strftime("%Y-%m-%d %H:%M"), a.size, a.font, build_id, a.scenario, a.display,
            os.path.relpath(profile, ROOT)))
        for label, path in shots:
            f.write("- `%s`: %s\n" % (os.path.relpath(path, ROOT), label))
    print("done: %d captures" % len(shots))
    return 0


if __name__ == "__main__":
    sys.exit(main())
