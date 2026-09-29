#!/usr/bin/env python3
"""Native interaction checks for the Astral UI overhaul.

Drives the real client on a private Xvfb display and clicks widgets *by name* using the
widget probe (`CDDA_UI_PROBE=<file>`: every shared primitive records its label and screen
rectangle each frame). Each check records PASS/FAIL plus a screenshot; results go to
`<out>/interaction-<tag>.md`.

    tools/ui-art/interact.py --size 3840x2160 --font 24 --out artifacts/ui-art-overhaul/m3 \
        --scenario equipment [--tag 3840x2160]

Scenarios:
  equipment  keyboard expand/collapse + Escape ownership, take off, equip from the inventory
             list, drag & drop with preview + Apply change, right-click context menu, close
  menu       main-menu overlay: click categories, drawer rows, hotkeys still work
  hud        toolbar buttons open the matching screens; sidebar buttons
"""
import argparse
import json
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from capture import (Client, LOGS, ROOT, Probe, dismiss_debug_prompts, ensure_hud, prepare_profile, sh,  # noqa: E402
                     wait_for, wait_loaded, wait_stable, hud_visible)
from xdrive import XDrive  # noqa: E402


class RestartScenario(Exception):
    """Raised when the random start is unusable for the check (e.g. no carried storage)."""


class TracedDrive:
    """XDrive wrapper that records every input sent to the client (for the report) and
    stretches mouse clicks to the client's frame time: the SDL3/ImGui client only sees a
    press and a release when they land in different frames, and the software renderer at
    3840x2160 runs at a few frames per second."""

    def __init__(self, x, rep, probe):
        self._x, self._rep, self._probe = x, rep, probe

    def click(self, x, y, button=1, hold=None, settle=None):
        ft = self._probe.frame_dt
        hold = max(hold or 0.15, 2.0 * ft)
        settle = max(settle or 0.15, 1.5 * ft)
        self._rep.trace.append("click %d %d button=%d hold=%.2f" % (x, y, button, hold))
        self._x.move(x, y, settle=max(0.3, 2.0 * ft))
        return self._x.click(x, y, button=button, hold=hold, settle=settle)

    def type_text(self, text, delay=None):
        ft = self._probe.frame_dt
        self._rep.trace.append("type_text %s" % text)
        return self._x.type_text(text, delay=max(delay or 0.05, 0.6 * ft))

    def press(self, x, y):
        ft = self._probe.frame_dt
        self._rep.trace.append("press %d %d" % (x, y))
        self._x.move(x, y, settle=max(0.3, 2.0 * ft))
        self._x.xtst.XTestFakeButtonEvent(self._x.dpy, 1, 1, 0)
        self._x.x11.XFlush(self._x.dpy)
        time.sleep(max(0.2, 2.0 * ft))

    def release(self):
        ft = self._probe.frame_dt
        self._rep.trace.append("release")
        self._x.xtst.XTestFakeButtonEvent(self._x.dpy, 1, 0, 0)
        self._x.x11.XFlush(self._x.dpy)
        time.sleep(max(0.8, 2.0 * ft))

    def __getattr__(self, name):
        attr = getattr(self._x, name)
        if name in ("key", "drag", "move", "scroll") and callable(attr):
            def traced(*a, **kw):
                self._rep.trace.append("%s %s %s" % (name, " ".join(str(v) for v in a),
                                                     " ".join("%s=%s" % kv for kv in kw.items())))
                return attr(*a, **kw)
            return traced
        return attr


class Report:
    def __init__(self, out, tag, scenario="run"):
        self.out, self.tag, self.scenario = out, tag, scenario
        self.lines = []
        self.shots = []
        self.trace = []
        self.n = 0

    def check(self, name, ok, detail=""):
        self.trace.append("CHECK %s: %s" % ("PASS" if ok else "FAIL", name))
        self.lines.append(("PASS" if ok else "FAIL", name, detail))
        print("  [%s] %s %s" % ("PASS" if ok else "FAIL", name, ("— " + detail) if detail else ""))
        return ok

    def note(self, text):
        self.lines.append(("NOTE", text, ""))
        print("  note:", text)

    def shot(self, x, label, settle=0.8):
        self.n += 1
        path = os.path.join(self.out, "interact-%s-%02d-%s-%s.png" % (self.scenario, self.n, label, self.tag))
        self.trace.append("SHOT %s" % label)
        x.shot(path, settle=settle)
        self.shots.append((label, path))
        print("  captured", path)
        return path

    def write(self, scenario, size, font, build_id, profile, telemetry_lines):
        path = os.path.join(self.out, "interaction-%s-%s.md" % (scenario, self.tag))
        passed = sum(1 for s, _, _ in self.lines if s == "PASS")
        failed = sum(1 for s, _, _ in self.lines if s == "FAIL")
        with open(path, "w") as f:
            f.write("# Native interaction check: %s @ %s (font %d)\n\n" % (scenario, size, font))
            f.write("Build `%s`, %s, profile `%s`, display Xvfb, software renderer. "
                    "Widgets located through the `CDDA_UI_PROBE` widget probe and driven with XTest "
                    "(real mouse/keyboard events into the real client).\n\n" % (
                        build_id, time.strftime("%Y-%m-%d %H:%M"), os.path.relpath(profile, ROOT)))
            f.write("**%d passed, %d failed.**\n\n" % (passed, failed))
            f.write("| Result | Check | Detail |\n|---|---|---|\n")
            for status, name, detail in self.lines:
                f.write("| %s | %s | %s |\n" % (status, name.replace("|", "\\|"), detail.replace("|", "\\|")))
            f.write("\n## Screenshots\n\n")
            for label, p in self.shots:
                f.write("- `%s`: %s\n" % (os.path.relpath(p, ROOT), label))
            f.write("\n## Input trace\n\n```\n")
            for line in self.trace:
                f.write(line + "\n")
            f.write("```\n")
            if telemetry_lines:
                f.write("\n## Telemetry (equipment actions)\n\n```\n")
                for line in telemetry_lines[-40:]:
                    f.write(line.rstrip() + "\n")
                f.write("```\n")
        print("report:", path)
        return path


def start_game(x, probe, rep):
    print("waiting for the main menu…")
    wait_stable(x, seconds=3, max_wait=240)
    x.key("n")
    time.sleep(0.6)
    x.key("o")  # Play Now!
    time.sleep(2)
    print("waiting for world generation / load…")
    wait_loaded(x, probe)
    rep.check("new game reaches the HUD (opening dialogue / debug prompts handled)",
              ensure_hud(x, probe, lambda label: rep.shot(x, label)))


def equipment_open(p):
    return bool(p.widgets("inv_row*") or p.widgets("button", "Equip", True))


def scenario_equipment(x, probe, rep):
    def norm(t):
        return "".join(c for c in t.lower() if c.isalnum())

    def inv_count(p):
        return len(p.widgets("inv_row*", visible_only=False))

    def in_equipment(p, w):
        win = p.first("window", "Equipment")
        if not win:
            return True
        cx, cy = Probe.center(w)
        return win["min"][0] <= cx <= win["max"][0] and win["min"][1] <= cy <= win["max"][1]

    def tree_rows(p):
        return [w for w in p.widgets("row*") if in_equipment(p, w)]

    start_game(x, probe, rep)
    x.key("i")
    opened = wait_for(probe, equipment_open, 30)
    ft = probe.frame_time()
    rep.note("client frame time %.2f s (%.1f fps) — clicks are stretched to match" % (ft, 1.0 / max(1e-3, ft)))
    rep.check("equipment window opens with 'i'", opened,
              "%d tree rows, %d inventory rows" % (len(probe.widgets("row*")), len(probe.widgets("inv_row*"))))
    dismiss_debug_prompts(x)
    probe.read()
    rows0 = len(tree_rows(probe))
    inv0 = len(probe.widgets("inv_row*"))
    if inv0 == 0 and rows0 > 0:
        raise RestartScenario("random character has no carried storage (0 inventory rows)")
    rep.check("inventory list visible beside the equipment tree (side by side)",
              inv0 > 0 and rows0 > 0, "%d rows / %d inventory rows" % (rows0, inv0))
    rep.shot(x, "equipment-initial")

    # --- keyboard: Down Down Right expands, Escape returns to overview without closing
    x.key("Down"); time.sleep(0.3); x.key("Down"); time.sleep(0.3); x.key("Right"); time.sleep(0.6)
    probe.read()
    rows1 = len(tree_rows(probe))
    rep.check("keyboard Right expands the focused group", rows1 > rows0, "%d → %d rows" % (rows0, rows1))
    rep.shot(x, "keyboard-expanded")
    x.key("Escape"); time.sleep(0.8)
    probe.read()
    rows2 = len(tree_rows(probe))
    rep.check("Escape collapses to the overview but keeps the window open",
              rows2 == rows0 and equipment_open(probe), "%d rows after Escape, window %s" % (
                  rows2, "open" if equipment_open(probe) else "closed"))

    # --- mouse: expand a group, select a worn item, take it off
    groups = tree_rows(probe)
    taken_name = None
    slot_row = None
    group_row = None
    for g in groups:
        x.click(*Probe.center(g)); probe.read(0.6)
        new_rows = [r for r in tree_rows(probe) if r["label"] not in {gg["label"] for gg in groups}]
        if not new_rows:
            continue
        for r in new_rows:
            x.click(*Probe.center(r)); probe.read(0.6)
            takeoff = probe.first("button", "Take off", True)
            status = probe.first("status")
            if takeoff and status and status["label"] and "Select an item" not in status["label"] \
                    and " > " not in status["label"] and not status["label"].rstrip().endswith(("(left)", "(right)")):
                # skip containers with contents and held items: taking those off can drop them
                taken_name = status["label"]
                slot_row = r
                group_row = g
                break
        if taken_name:
            break
        x.click(*Probe.center(g)); probe.read(0.4)  # collapse again
    rep.check("clicking a slot row selects its worn item (Take off enabled)", taken_name is not None,
              "%s (row '%s')" % (taken_name, slot_row["label"]) if taken_name else "no worn item found")
    if not taken_name:
        return
    rep.shot(x, "slot-selected")
    inv_before = inv_count(probe)
    x.click(*Probe.center(probe.first("button", "Take off", True)))
    time.sleep(1.5)
    dismiss_debug_prompts(x)
    probe.read()
    # display_name adds prefixes/suffixes ("++ jeans (fits)"); inventory rows show type_name.
    # "++ white dress shirt > permanent marker (fits)": the worn item is the part before " > ".
    base = norm(taken_name.split(" > ")[0])

    def inv_match(p, visible_only=True):
        rows = [w for w in p.widgets("inv_row*", visible_only=visible_only)
                if norm(w["label"]) and norm(w["label"]) in base]
        rows.sort(key=lambda w: -len(norm(w["label"])))
        return rows
    def status_text(p):
        st = p.first("status")
        return st["label"] if st else ""

    ok = wait_for(probe, lambda p: len(inv_match(p, False)) > 0 and (
        inv_count(p) > inv_before or "taken off" in status_text(p).lower()), 8)
    status = probe.first("status")
    dropped = not ok and status and "taken off" in status["label"].lower()
    rep.check("Take off moves the item into the inventory list", ok or dropped,
              "status: %s; inventory rows %d → %d%s" % (status["label"] if status else "-", inv_before,
                                                          inv_count(probe),
                                                          " (no room to carry it: dropped)" if dropped else ""))
    rep.shot(x, "after-takeoff")
    if not ok:
        rep.note("the item is not in the carried list, so the equip / drag steps are skipped this run")
        return
    item_label = inv_match(probe, False)[0]["label"]

    def set_filter(text):
        field = probe.first("input", "inventory_filter", True)
        if not field:
            return False
        x.click(*Probe.center(field)); time.sleep(0.3)
        # Clear with End + one BackSpace per character, spaced by a frame: Ctrl+A and rapid
        # key bursts lose events when several land in one slow (4K software) frame.
        x.key("End")
        for _ in range(len(set_filter.current) + 2):
            x.key("BackSpace", hold=max(0.03, probe.frame_dt * 1.2))
        set_filter.current = text
        if text:
            x.type_text(text)
        time.sleep(0.4)
        # leave the field so Escape/hotkeys go to the window again
        blank = probe.first("window")
        x.click(blank["min"][0] + 30, blank["min"][1] + 30) if blank else None
        probe.read(0.5)
        return True

    set_filter.current = ""

    # --- search filter narrows the list to the item
    word = item_label.split()[0]
    rep.check("search field filters the inventory list", set_filter(word) and
              all(word.lower() in w["label"].lower() for w in probe.widgets("inv_row*")) and inv_match(probe),
              "filter '%s' → %d rows" % (word, len(probe.widgets("inv_row*"))))
    rep.shot(x, "inventory-filtered")

    # --- equip from the inventory list (click row, click Equip)
    x.click(*Probe.center(inv_match(probe)[0]))
    wait_for(probe, lambda p: p.first("inv_row_selected") is not None, 5)
    sel = probe.first("inv_row_selected")
    rep.check("clicking an inventory row selects it", sel is not None and sel["label"] == item_label,
              sel["label"] if sel else "no selected row")
    inv_before = inv_count(probe)
    ok = False
    for attempt in range(2):
        x.click(*Probe.center(probe.first("button", "Equip", True)), hold=0.2)
        time.sleep(1.5)
        dismiss_debug_prompts(x)
        ok = wait_for(probe, lambda p: inv_count(p) < inv_before or status_text(p).lower().startswith("equipped"), 5)
        if ok:
            if attempt:
                rep.note("Equip needed a second click (first click not registered)")
            break
    status = probe.first("status")
    rep.check("Equip (primary action) wears the selected inventory item", ok,
              "status: %s; inventory rows %d → %d" % (status["label"] if status else "-", inv_before,
                                                        inv_count(probe)))
    rep.shot(x, "after-equip")

    # --- drag & drop onto the slot row: preview + Apply change
    # The slot the item actually went to is in the status ("Equipped to Neck."); take it off
    # from that row so the drag target is the same row.
    status = probe.first("status")
    dest_label = slot_row["label"]
    if status and status["label"].lower().startswith("equipped to "):
        dest_label = status["label"][len("Equipped to "):].rstrip(".").strip()
    set_filter("")

    def find_row(label):
        r = probe.first("row*", label, True)
        if r:
            return r
        for g in tree_rows(probe):
            x.click(*Probe.center(g)); probe.read(0.6)
            r = probe.first("row*", label, True)
            if r:
                return r
        return None

    slot_now = find_row(dest_label)
    if slot_now:
        x.click(*Probe.center(slot_now)); probe.read(0.6)
        to = probe.first("button", "Take off", True)
        if to:
            x.click(*Probe.center(to)); time.sleep(1.5); dismiss_debug_prompts(x)
            wait_for(probe, lambda p: len(inv_match(p, False)) > 0, 8)
    set_filter(word)
    slot_row = {"label": dest_label}
    src = inv_match(probe)
    dst = probe.first("row*", slot_row["label"], True)
    if not dst:
        dst = find_row(slot_row["label"])
        src = inv_match(probe)
    if src and dst:
        sx, sy = Probe.center(src[0])
        dx, dy = Probe.center(dst)
        x.press(sx, sy)
        step = max(0.06, probe.frame_dt * 0.5)
        for i in range(1, 16):
            t = i / 15.0
            x.move(sx + (dx - sx) * t, sy + (dy - sy) * t, settle=step)
        time.sleep(max(0.3, 2 * probe.frame_dt))
        rep.shot(x, "drag-in-progress", settle=0.1)
        x.release()
        probe.read()
        apply_btn = probe.first("button", "Apply change", True)
        rep.check("drag & drop onto a slot shows the preview with Apply change / Cancel change",
                  apply_btn is not None and probe.first("button", "Cancel change", True) is not None)
        rep.shot(x, "drag-preview")
        if apply_btn:
            inv_before = inv_count(probe)
            x.click(*Probe.center(apply_btn)); time.sleep(1.5); dismiss_debug_prompts(x)
            ok = wait_for(probe, lambda p: inv_count(p) < inv_before or
                          status_text(p).lower().startswith("equipped"), 8)
            status = probe.first("status")
            rep.check("Apply change equips the dragged item", ok,
                      "status: %s" % (status["label"] if status else "-"))
            rep.shot(x, "after-apply")
    else:
        st = probe.first("status")
        rep.shot(x, "drag-setup-failed")
        rep.check("drag & drop setup (item back in inventory, slot row visible)", False,
                  "src=%s dst=%s; status: %s; carried rows: %d (%s)" % (
                      bool(src), bool(dst), st["label"] if st else "-", inv_count(probe),
                      ", ".join(w["label"] for w in probe.widgets("inv_row*", visible_only=False)[:8])))

    # --- context menu on an inventory row
    set_filter("")
    inv = probe.widgets("inv_row*")
    if inv:
        x.click(*Probe.center(inv[0]), button=3); time.sleep(0.6)
        menu_shot = rep.shot(x, "inventory-context-menu")
        x.key("Escape"); time.sleep(max(0.5, 2 * probe.frame_dt))
        probe.read()
        after = rep.shot(x, "after-context-escape", settle=0.2)
        menu_gone = image_diff(menu_shot, after) > 0.0005
        if not menu_gone:
            # ImGui popups also close on a click outside them; record which worked.
            blank = probe.first("window")
            if blank:
                x.click(blank["min"][0] + 30, blank["min"][1] + 30)
            probe.read()
            after2 = rep.shot(x, "after-context-click-outside", settle=0.2)
            rep.note("context menu ignored Escape at %.1f fps; closed by clicking outside instead" % (
                1.0 / max(1e-3, probe.frame_dt)))
            menu_gone = image_diff(menu_shot, after2) > 0.0005
        rep.check("context menu closes (Escape, or click outside) without closing the window",
                  menu_gone and equipment_open(probe))

    # --- Escape ownership on the way out: one press per level (group → overview → close)
    presses = 0
    states = []
    for _ in range(3):
        x.key("Escape")
        presses += 1
        # wait for the client to show the result of this press before pressing again
        closed = wait_for(probe, lambda p: not equipment_open(p), 6)
        states.append("closed" if closed else "open")
        if closed:
            break
    back = wait_for(probe, hud_visible, 10)
    rep.check("Escape closes the window one level at a time (group → overview → close), never the game",
              not equipment_open(probe) and back,
              "%d press(es): %s; HUD toolbar back: %s" % (presses, " → ".join(states), back))
    rep.shot(x, "after-close")


def scenario_menu(x, probe, rep):
    print("waiting for the main menu…")
    wait_stable(x, seconds=3, max_wait=240)
    ok = wait_for(probe, lambda p: p.first("row*", "New Game") is not None, 20)
    rep.note("client frame time %.2f s" % probe.frame_time())
    rep.check("main-menu overlay renders category rows", ok, ", ".join(w["label"] for w in probe.widgets("row*")[:9]))
    rep.shot(x, "menu-initial")
    for label, expect in (("Load", None), ("Settings", "Options"), ("World", None), ("New Game", "Play Now")):
        row = probe.first("row*", label)
        if not row:
            rep.check("row %s present" % label, False)
            continue
        x.click(*Probe.center(row)); probe.read(0.6)
        sel = probe.first("row_selected")
        drawer = [w["label"] for w in probe.widgets("row*") if w is not row]
        good = sel is not None and label.lower() in sel["label"].lower()
        if expect:
            good = good and any(expect.lower() in d.lower() for d in drawer)
        rep.check("click '%s' selects it and opens its drawer" % label, good,
                  "selected=%s drawer=%s" % (sel["label"] if sel else "-", ", ".join(drawer[:8])))
        rep.shot(x, "menu-" + label.lower().replace(" ", "-"))
    # hotkey still works with the overlay (t = Settings)
    x.key("t"); probe.read(0.6)
    sel = probe.first("row_selected")
    rep.check("hotkey 't' still selects Settings under the overlay", sel is not None and "Settings" in sel["label"],
              sel["label"] if sel else "-")
    # drawer row click: Settings → Options opens the (curses) options screen. Curses screens
    # render no ImGui frames, so the probe goes stale there; compare screenshots instead.
    opt = probe.first("row*", "Options")
    if opt:
        before = os.path.join(rep.out, "_menu_before.png")
        x.shot(before, settle=0.2)
        x.click(*Probe.center(opt)); time.sleep(2.5)
        after = rep.shot(x, "menu-options-opened")
        rep.check("clicking a drawer row (Options) opens that screen", image_diff(before, after) > 0.06,
                  "changed pixels: %.0f%%" % (100 * image_diff(before, after)))
        os.remove(before)
        x.key("Escape"); time.sleep(1.5)
        wait_for(probe, lambda p: p.first("row*", "New Game") is not None, 20)
        rep.check("Escape returns to the main menu overlay", probe.first("row*", "New Game") is not None)
        rep.shot(x, "menu-after-options")


def image_diff(a, b):
    """Fraction of pixels that differ noticeably between two screenshots (downsampled)."""
    from PIL import Image, ImageChops
    ia = Image.open(a).convert("L").resize((320, 180))
    ib = Image.open(b).convert("L").resize((320, 180))
    d = ImageChops.difference(ia, ib).point(lambda v: 255 if v > 24 else 0)
    return sum(1 for v in d.getdata() if v) / float(320 * 180)


def close_and_return(x, probe, opener):
    """Press Escape one level at a time until the screen is closed and the HUD is back."""
    for _ in range(3):
        x.key("Escape")
        if wait_for(probe, lambda p: not opener(p) and hud_visible(p), 6):
            return True
    return False


def scenario_hud(x, probe, rep):
    start_game(x, probe, rep)
    ok = wait_for(probe, lambda p: p.first("toolbar", "Inv") is not None, 20)
    rep.note("client frame time %.2f s" % probe.frame_time())
    rep.check("mouse toolbar renders with probe-visible buttons", ok,
              ", ".join(w["label"] for w in probe.widgets("toolbar")[:16]))
    rep.shot(x, "hud-initial")
    for label, opener in (("Inv", equipment_open), ("Char", lambda p: p.first("window", "Character") is not None),
                          ("Craft", lambda p: p.first("window", "Craft") is not None)):
        b = probe.first("toolbar", label, True)
        if not b:
            rep.check("toolbar button %s present" % label, False)
            continue
        x.click(*Probe.center(b)); time.sleep(1.0); dismiss_debug_prompts(x)
        ok = wait_for(probe, opener, 30)
        rep.check("toolbar '%s' opens its screen" % label, ok)
        rep.shot(x, "toolbar-" + label.lower())
        rep.check("Escape closes '%s' and returns to the HUD" % label, close_and_return(x, probe, opener))
    # sidebar: Gear button opens equipment, Health opens the medical screen
    for label, opener in (("Gear", equipment_open), ("Health", lambda p: p.first("window", "Medical") is not None)):
        b = probe.first("button", label, True)
        if not b:
            rep.check("sidebar button %s present" % label, False)
            continue
        x.click(*Probe.center(b)); time.sleep(1.0); dismiss_debug_prompts(x)
        ok = wait_for(probe, opener, 30)
        rep.check("sidebar '%s' opens its screen" % label, ok)
        rep.shot(x, "sidebar-" + label.lower())
        rep.check("Escape closes '%s' and returns to the HUD" % label, close_and_return(x, probe, opener))
    rep.shot(x, "hud-final")


SCENARIOS = {"equipment": scenario_equipment, "menu": scenario_menu, "hud": scenario_hud}


def run_once(a, out, tag, profile, attempt):
    if os.path.isdir(profile):
        import shutil
        shutil.rmtree(profile)
    prepare_profile(profile, a.font)
    probe_path = os.path.join(profile, "config", "ui-probe.json")
    cli = Client(a.size, a.display, profile, a.binary, {"CDDA_UI_PROBE": probe_path}).start()
    time.sleep(3)
    if not cli.alive():
        print("client did not start; see", LOGS)
        return None, None
    probe = Probe(probe_path)
    rep = Report(out, tag, a.scenario)
    if attempt > 1:
        rep.note("attempt %d (previous random start was unusable)" % attempt)
    x = TracedDrive(XDrive(a.display), rep, probe)
    probe.nudge = x.nudge
    restart = False
    try:
        SCENARIOS[a.scenario](x, probe, rep)
    except RestartScenario as e:
        rep.note("restarting: %s" % e)
        restart = True
    except Exception as e:  # keep partial evidence
        rep.check("scenario completed without harness errors", False, repr(e))
        rep.shot(x, "harness-error")
    finally:
        if not a.keep or restart:
            cli.stop()
    return rep, restart


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--size", default="1280x800")
    ap.add_argument("--out", required=True)
    ap.add_argument("--binary", default=os.path.join(ROOT, "build", "src", "cataclysm-tiles"))
    ap.add_argument("--display", default=":99")
    ap.add_argument("--scenario", default="equipment", choices=sorted(SCENARIOS))
    ap.add_argument("--font", type=int, default=16)
    ap.add_argument("--tag", default=None)
    ap.add_argument("--keep", action="store_true")
    ap.add_argument("--attempts", type=int, default=3, help="restarts allowed for an unusable random start")
    a = ap.parse_args()
    out = os.path.abspath(a.out)
    os.makedirs(out, exist_ok=True)
    tag = a.tag or a.size
    profile = os.path.abspath(os.path.join(ROOT, "artifacts", "ui-art-overhaul", "profiles",
                                           "interact-%s-%s" % (a.scenario, a.size)))
    build_id = sh("cd %s && git rev-parse --short HEAD" % ROOT).stdout.strip()
    rep = None
    for attempt in range(1, a.attempts + 1):
        for f in os.listdir(out):
            if f.startswith("interact-%s-" % a.scenario) and f.endswith("-%s.png" % tag):
                os.remove(os.path.join(out, f))
        rep, restart = run_once(a, out, tag, profile, attempt)
        if rep is None:
            return 1
        if not restart:
            break
    telem = []
    tpath = os.path.join(profile, "config", "ui-telemetry.jsonl")
    if os.path.exists(tpath):
        with open(tpath) as f:
            telem = [line for line in f if "equipment" in line]
    rep.write(a.scenario, a.size, a.font, build_id, profile, telem)
    return 0 if all(s != "FAIL" for s, _, _ in rep.lines) else 2


if __name__ == "__main__":
    sys.exit(main())
