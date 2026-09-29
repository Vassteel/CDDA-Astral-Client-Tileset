#!/usr/bin/env python3
"""Verified, reversible Linux/SteamOS updates for the Astral Client (standard library only)."""
import argparse
import contextlib
import fcntl
import hashlib
import json
import os
import re
from pathlib import Path, PurePosixPath
import shutil
import stat
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.request
import zipfile

LIMIT = 512 * 1024 * 1024
REPOSITORY = "Vassteel/CDDA-Astral-Client-Tileset"
USER_AGENT = "Astral-Client-Updater"
TITLE = "Astral Client Update"
RETRIES = 3

class UpdateError(Exception):
    pass

def digest(path):
    value = hashlib.sha256()
    with open(path, "rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(chunk)
    return value.hexdigest()

def relative_path(value):
    if not isinstance(value, str) or not value or "\x00" in value:
        raise UpdateError("Invalid package path")
    p = PurePosixPath(value)
    if "\\" in value or p.is_absolute() or any(x in ("", ".", "..") for x in value.split("/")):
        raise UpdateError("Invalid package path")
    # This version updates executable/title assets only, never player data,
    # user mods, tilesets, configuration, libraries or launch scripts.
    if value != "cataclysm-tiles" and not (value.startswith("data/title/") and p.suffix.lower() in (".png", ".jpg", ".jpeg")):
        raise UpdateError(f"Unsupported update target: {value}")
    return p

def destination(client, name):
    p = relative_path(name)
    target = client.joinpath(*p.parts)
    for ancestor in [target, *target.parents]:
        if ancestor == client:
            break
        if ancestor.is_symlink():
            raise UpdateError(f"Update target contains a symlink: {name}")
    if target.exists() and not target.is_file():
        raise UpdateError(f"Update target is not a file: {name}")
    return target

def running(client):
    """Check executable mappings too: /proc/exe can name a replaced executable."""
    client = client.resolve()
    wanted = str(client / "cataclysm-tiles")
    found = []
    for proc in Path("/proc").iterdir():
        if not proc.name.isdecimal():
            continue
        try:
            exe = os.readlink(proc / "exe").removesuffix(" (deleted)")
            if exe == wanted:
                found.append(int(proc.name))
                continue
            cmd = (proc / "cmdline").read_bytes().split(b"\0")
            if cmd and b"cataclysm" in cmd[0] and Path(os.readlink(proc / "cwd")).resolve() == client:
                found.append(int(proc.name))
        except (FileNotFoundError, ProcessLookupError):
            continue
        except PermissionError:
            # Other users' processes do not own this user's client session.
            continue
    return found

def require_closed(client):
    if running(client):
        raise UpdateError("CDDA is running. The update remains staged; save and exit before installing.")

def inspect_package(package, extract=None):
    with zipfile.ZipFile(package) as archive:
        infos = archive.infolist()
        names = [i.filename for i in infos]
        if len(names) != len(set(names)) or sum(i.file_size for i in infos) > LIMIT:
            raise UpdateError("Duplicate archive entries or oversized update")
        if "update.json" not in names or archive.getinfo("update.json").file_size > 1024 * 1024:
            raise UpdateError("Missing or oversized update manifest")
        manifest = json.loads(archive.read("update.json"))
        if not isinstance(manifest, dict):
            raise UpdateError("Invalid update manifest")
        if manifest.get("schema") != 1 or manifest.get("platform") != "linux-x86_64" or not isinstance(manifest.get("version"), str):
            raise UpdateError("Unsupported update format/platform")
        files = manifest.get("files", [])
        if not isinstance(files, list) or not files or len(files) > 128:
            raise UpdateError("Invalid update file list")
        expected = {"update.json"}
        for entry in files:
            if not isinstance(entry, dict) or not isinstance(entry.get("size"), int) or entry["size"] < 0 or not isinstance(entry.get("sha256"), str) or not re.fullmatch(r"[0-9a-f]{64}", entry["sha256"]):
                raise UpdateError("Invalid payload metadata")
            name = entry["path"]
            relative_path(name)
            member = "payload/" + name
            if member in expected:
                raise UpdateError("Duplicate manifest path")
            expected.add(member)
            info = archive.getinfo(member)
            if stat.S_ISLNK(info.external_attr >> 16) or info.file_size != entry["size"]:
                raise UpdateError("Invalid payload type or size")
            h = hashlib.sha256()
            with archive.open(member) as source:
                output = open(Path(extract) / name, "wb") if extract and name == "cataclysm-tiles" else None
                if extract and output is None:
                    target = Path(extract) / name
                    target.parent.mkdir(parents=True, exist_ok=True)
                    output = open(target, "wb")
                try:
                    for chunk in iter(lambda: source.read(1024 * 1024), b""):
                        h.update(chunk)
                        if output:
                            output.write(chunk)
                finally:
                    if output:
                        output.close()
            if h.hexdigest() != entry["sha256"]:
                raise UpdateError(f"Checksum mismatch: {name}")
        if set(names) != expected or "payload/cataclysm-tiles" not in expected:
            raise UpdateError("Unexpected or missing archive members")
        return manifest

def atomic_copy(source, target, mode):
    target.parent.mkdir(parents=True, exist_ok=True)
    fd, temporary = tempfile.mkstemp(prefix=".hybrid-update-", dir=target.parent)
    try:
        with os.fdopen(fd, "wb") as out, open(source, "rb") as src:
            shutil.copyfileobj(src, out)
            out.flush()
            os.fsync(out.fileno())
        os.chmod(temporary, mode)
        os.replace(temporary, target)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)

def write_json(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(".tmp")
    with open(temporary, "w") as out:
        out.write(json.dumps(value, indent=2) + "\n")
        out.flush()
        os.fsync(out.fileno())
    os.replace(temporary, path)

def parse_version(tag):
    """client-v0.1.2 -> (0, 1, 2); None when the tag is not a client version."""
    match = re.fullmatch(r"client-v(\d+(?:\.\d+)*)", tag or "")
    return tuple(int(x) for x in match.group(1).split(".")) if match else None

def is_current(installed, latest):
    """Never offer a downgrade: a newer local build counts as up to date."""
    have, want = parse_version(installed), parse_version(latest)
    if have is None or want is None:
        return installed == latest
    return have >= want

def default_state(client):
    """Per-installation state beside the client, like the Windows updater.

    Older releases shared "hybrid-updates" between every client in a folder,
    which blocked a newly extracted full download. Keep using it only when it
    already belongs to this installation so rollback history is not lost."""
    legacy = client.parent / "hybrid-updates"
    with contextlib.suppress(OSError, ValueError, KeyError, TypeError):
        if json.loads((legacy / "installed.json").read_text())["client"] == str(client):
            return legacy
    return client.parent / f".{client.name}-updates"

def installed_version(client, state):
    """The verified version of the current files, or None when unknown/modified."""
    marker = state / "installed.json"
    if marker.is_file():
        info = json.loads(marker.read_text())
        if info.get("client") == str(client):
            try:
                if all(digest(destination(client, f["path"])) == f["sha256"] for f in info.get("files", [])):
                    return info.get("version")
            except (OSError, UpdateError):
                pass
            return None
    version_file = client / "VERSION.json"
    if version_file.is_file():
        info = json.loads(version_file.read_text())
        if info.get("version") and (client / "cataclysm-tiles").is_file() and info.get("executable_sha256") == digest(client / "cataclysm-tiles"):
            return "client-v" + info["version"]
    return None

def rollback_chain(state):
    """Backups still reachable from installed.json through previous installs."""
    chain = set()
    marker = state / "installed.json"
    current = json.loads(marker.read_text()) if marker.exists() else None
    while current and current.get("backup") and current["backup"] not in chain:
        chain.add(current["backup"])
        try:
            current = json.loads((Path(current["backup"]) / "backup.json").read_text()).get("previous_install")
        except (OSError, ValueError):
            break
    return chain

def prune(state, keep=None):
    """Remove finished downloads and backups rollback can no longer reach.

    Backups of an interrupted install ("prepared") are always kept for manual
    recovery, as is every backup in the rollback chain."""
    for staged in [*state.glob("release-*.zip"), *state.glob("release-*.download")]:
        if keep is None or staged.resolve() != Path(keep).resolve():
            staged.unlink(missing_ok=True)
    chain = rollback_chain(state)
    for backup in (state / "backups").glob("*") if (state / "backups").is_dir() else []:
        try:
            status = json.loads((backup / "backup.json").read_text()).get("status")
        except (OSError, ValueError):
            continue
        if status in ("rolled-back", "failed-restored") and str(backup) not in chain:
            shutil.rmtree(backup, ignore_errors=True)

@contextlib.contextmanager
def locked(state):
    state.mkdir(parents=True, exist_ok=True)
    with open(state / "update.lock", "a") as lock:
        try:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError as error:
            raise UpdateError("Another updater is already active") from error
        yield

def restore(client, backup, record, changed=None):
    entries = record["files"] if changed is None else changed
    for entry in reversed(entries):
        target = destination(client, entry["path"])
        if entry["existed"]:
            source = backup / "files" / entry["path"]
            if digest(source) != entry["sha256"]:
                raise UpdateError("Backup checksum mismatch; no unverified restore performed")
            atomic_copy(source, target, entry["mode"])
        elif target.exists():
            target.unlink()

def validate_state(client, state):
    if state.resolve() == client.resolve() or client.resolve() in state.resolve().parents:
        raise UpdateError("Updater state must be outside the client installation")

def apply(package, client, state):
    validate_state(client, state)
    client = client.resolve()
    if not (client / "cataclysm-tiles").is_file():
        raise UpdateError("Select an existing CDDA client installation")
    with locked(state):
        # Validate all bytes outside the installation before touching it.
        with tempfile.TemporaryDirectory(prefix="verify-", dir=state) as temporary:
            manifest = inspect_package(package, temporary)
            targets = [(entry, destination(client, entry["path"])) for entry in manifest["files"]]
            require_closed(client)
            backup = state / "backups" / str(time.time_ns())
            record = {"client": str(client), "version": manifest["version"], "status": "prepared", "files": []}
            installed = state / "installed.json"
            record["previous_install"] = json.loads(installed.read_text()) if installed.exists() else None
            if record["previous_install"] and record["previous_install"]["client"] != str(client):
                raise UpdateError("Updater state belongs to another installation")
            for entry, target in targets:
                before = {"path": entry["path"], "existed": target.exists(), "installed_sha256": entry["sha256"]}
                if target.exists():
                    before.update(sha256=digest(target), mode=stat.S_IMODE(target.stat().st_mode))
                    saved = backup / "files" / entry["path"]
                    saved.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copy2(target, saved)
                record["files"].append(before)
            write_json(backup / "backup.json", record)
            changed = []
            try:
                # Recheck after preparing the backup as download/verification
                # can take time. Never kill a game process.
                require_closed(client)
                for entry, target in targets:
                    atomic_copy(Path(temporary) / entry["path"], target, 0o755 if entry["path"] == "cataclysm-tiles" else 0o644)
                    changed.append(next(x for x in record["files"] if x["path"] == entry["path"]))
                write_json(installed, {"version": manifest["version"], "client": str(client), "backup": str(backup), "files": manifest["files"]})
            except Exception:
                restore(client, backup, record, changed)
                record["status"] = "failed-restored"
                write_json(backup / "backup.json", record)
                raise
            record["status"] = "installed"
            write_json(backup / "backup.json", record)
            return {"version": manifest["version"], "backup": str(backup)}

def rollback(client, state):
    validate_state(client, state)
    client = client.resolve()
    with locked(state):
        installed = state / "installed.json"
        if not installed.exists():
            raise UpdateError("No updater-managed installation to roll back")
        current = json.loads(installed.read_text())
        if current["client"] != str(client.resolve()):
            raise UpdateError("Backup belongs to another installation")
        backup = Path(current["backup"])
        record = json.loads((backup / "backup.json").read_text())
        # Verify all backup bytes and current targets before changing anything.
        for entry in record["files"]:
            target = destination(client, entry["path"])
            if not target.exists() or digest(target) != entry["installed_sha256"]:
                raise UpdateError("Installed files changed since updating; rollback deferred")
            if entry["existed"] and digest(backup / "files" / entry["path"]) != entry["sha256"]:
                raise UpdateError("Backup checksum mismatch")
        require_closed(client)
        # Keep the current bytes too, so an I/O failure halfway through a
        # multi-file rollback can return to the complete installed version.
        with tempfile.TemporaryDirectory(prefix="rollback-", dir=state) as temporary:
            rollback_sources = []
            for entry in record["files"]:
                target = destination(client, entry["path"])
                saved = Path(temporary) / entry["path"]
                saved.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(target, saved)
                rollback_sources.append((saved, target, stat.S_IMODE(target.stat().st_mode)))
            require_closed(client)
            try:
                restore(client, backup, record)
                if record.get("previous_install"):
                    write_json(installed, record["previous_install"])
                else:
                    installed.unlink()
            except Exception:
                for source, target, mode in rollback_sources:
                    atomic_copy(source, target, mode)
                raise
        record["status"] = "rolled-back"
        write_json(backup / "backup.json", record)

def gh_binary():
    found = shutil.which("gh")
    if found:
        return found
    candidates = sorted((Path.home() / ".local/share/cdda-tools").glob("gh_*/bin/gh"))
    return str(candidates[-1]) if candidates else None

def http_open(url, accept="application/vnd.github+json"):
    """Open a GitHub URL, retrying transient network and server errors."""
    request = urllib.request.Request(url, headers={"User-Agent": USER_AGENT, "Accept": accept})
    for attempt in range(RETRIES):
        try:
            return urllib.request.urlopen(request, timeout=60)
        except urllib.error.HTTPError as error:
            if error.code == 403 and error.headers.get("X-RateLimit-Remaining") == "0":
                raise UpdateError("GitHub's hourly download-check limit was reached. Try again later.") from error
            if error.code < 500 or attempt == RETRIES - 1:
                raise UpdateError(f"GitHub returned HTTP {error.code} for {url}") from error
        except (urllib.error.URLError, TimeoutError, ConnectionError) as error:
            if attempt == RETRIES - 1:
                raise UpdateError("Could not reach GitHub. Check your internet connection and try again.") from error
        time.sleep(2 ** attempt)

def github_json(repo, suffix):
    # Public releases need no sign-in. An authenticated gh is only a fallback
    # (rate limits, private forks) so a logged-out gh never blocks updates.
    endpoint = f"repos/{repo}/{suffix}"
    try:
        with http_open("https://api.github.com/" + endpoint) as response:
            return json.load(response)
    except UpdateError:
        gh = gh_binary()
        if not gh:
            raise
        result = subprocess.run([gh, "api", endpoint], capture_output=True, text=True, timeout=60)
        if result.returncode:
            raise
        return json.loads(result.stdout)

def check(repo):
    # Tilesets have independent releases. Never mistake a newer art package for
    # a client update (or install a prerelease without an explicit request).
    releases = github_json(repo, "releases?per_page=100")
    for release in releases:
        if release.get("draft") or release.get("prerelease") or not release["tag_name"].startswith("client-v"):
            continue
        assets = [a for a in release["assets"] if a["name"].endswith("-linux-update.zip")]
        if len(assets) != 1 or not 0 < assets[0]["size"] <= LIMIT:
            return {"version": release["tag_name"], "url": release["html_url"], "full_download_required": True}
        return {"version": release["tag_name"], "url": release["html_url"], "asset": assets[0]}
    raise UpdateError("No supported Astral Client release is available yet")

def verify_download(path, release):
    asset = release["asset"]
    if path.stat().st_size != asset["size"]:
        raise UpdateError("Incomplete release download")
    expected = asset.get("digest")
    if expected and expected.startswith("sha256:") and digest(path) != expected[7:]:
        raise UpdateError("Release archive checksum mismatch")
    if inspect_package(path)["version"] != release["version"]:
        raise UpdateError("Release/package version mismatch")

def download(repo, release, state, progress=None):
    """Download and verify the update, reusing an already staged copy.

    progress(done, total) is called while downloading; it may raise to cancel."""
    if release.get("full_download_required"):
        raise UpdateError(f"{release['version']} requires the full client download: {release['url']}")
    state.mkdir(parents=True, exist_ok=True)
    asset = release["asset"]
    # The asset ID comes from authenticated GitHub metadata, never a local path.
    if not isinstance(asset["id"], int):
        raise UpdateError("Invalid release asset")
    target = state / f"release-{asset['id']}.zip"
    if target.is_file():
        # Staged earlier (e.g. while the game was running): no second download.
        try:
            verify_download(target, release)
            return target
        except (UpdateError, OSError, ValueError, KeyError, zipfile.BadZipFile):
            target.unlink(missing_ok=True)
    temporary = target.with_suffix(".download")
    url = asset["browser_download_url"]
    if not url.startswith(f"https://github.com/{repo}/releases/download/"):
        raise UpdateError("Unexpected release download URL")
    try:
        try:
            with open(temporary, "wb") as out, http_open(url, "application/octet-stream") as source:
                total = 0
                while chunk := source.read(1024 * 1024):
                    total += len(chunk)
                    if total > LIMIT:
                        raise UpdateError("Download exceeds size limit")
                    out.write(chunk)
                    if progress:
                        progress(total, asset["size"])
        except (UpdateError, OSError) as error:
            gh = gh_binary()
            if not gh or (isinstance(error, UpdateError) and str(error) == "Download exceeds size limit"):
                raise
            with open(temporary, "wb") as out:
                result = subprocess.run([gh, "api", "-H", "Accept: application/octet-stream", f"repos/{repo}/releases/assets/{asset['id']}"], stdout=out, stderr=subprocess.PIPE, timeout=600)
            if result.returncode:
                raise
        verify_download(temporary, release)
        os.replace(temporary, target)
        return target
    finally:
        temporary.unlink(missing_ok=True)

class Cancelled(Exception):
    pass

class Dialogs:
    """Zenity (SteamOS, GNOME), kdialog (KDE) or a terminal, in that order."""

    def __init__(self):
        self.kind = "zenity" if shutil.which("zenity") else "kdialog" if shutil.which("kdialog") else "terminal" if sys.stdin.isatty() else None
        if self.kind is None:
            raise UpdateError("The graphical updater needs zenity or kdialog. Run it from a terminal or use the check/download/apply commands.")

    def _run(self, zenity, kdialog):
        return subprocess.run(["zenity", f"--title={TITLE}", "--width=420", *zenity] if self.kind == "zenity" else ["kdialog", "--title", TITLE, *kdialog]).returncode

    def info(self, text):
        if self.kind == "terminal":
            print(text)
        else:
            self._run(["--info", "--text", text], ["--msgbox", text])

    def error(self, text):
        if self.kind == "terminal":
            print(text, file=sys.stderr)
        else:
            self._run(["--error", "--text", text], ["--error", text])

    def question(self, text, yes="Yes", no="No"):
        if self.kind == "terminal":
            try:
                return input(f"{text}\n[y/N] ").strip().lower() in ("y", "yes")
            except EOFError:
                return False
        return self._run(["--question", "--text", text, f"--ok-label={yes}", f"--cancel-label={no}"],
                         ["--yesno", text, "--yes-label", yes, "--no-label", no]) == 0

    @contextlib.contextmanager
    def progress(self, text):
        """Yield progress(done, total); raises Cancelled if the user cancels."""
        if self.kind != "zenity":
            def report(done, total):
                if self.kind == "terminal" and sys.stdout.isatty():
                    print(f"\r{text} {done * 100 // max(total, 1)}% ({done >> 20} / {total >> 20} MiB)", end="", flush=True)
            yield report
            if self.kind == "terminal" and sys.stdout.isatty():
                print()
            return
        window = subprocess.Popen(["zenity", "--progress", f"--title={TITLE}", "--width=420", "--auto-close", "--text", text],
                                  stdin=subprocess.PIPE, text=True)
        shown = [-1]
        def report(done, total):
            percent = min(99, done * 100 // max(total, 1))
            if window.poll() is not None:
                raise Cancelled()
            if percent != shown[0]:
                shown[0] = percent
                try:
                    window.stdin.write(f"{percent}\n# {text} {done >> 20} / {total >> 20} MiB\n")
                    window.stdin.flush()
                except BrokenPipeError as error:
                    raise Cancelled() from error
        try:
            yield report
        finally:
            with contextlib.suppress(BrokenPipeError, OSError):
                window.stdin.write("100\n")
                window.stdin.close()
            with contextlib.suppress(subprocess.TimeoutExpired):
                window.wait(timeout=5)
            if window.poll() is None:
                window.terminate()

def open_page(url):
    if url.startswith("https://github.com/") and shutil.which("xdg-open"):
        subprocess.Popen(["xdg-open", url], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        return True
    return False

def gui_update(repo, client, state, dialogs):
    release = check(repo)
    current = installed_version(client, state)
    label = current.removeprefix("client-v") if current else "an unrecognized version"
    if current and is_current(current, release["version"]):
        dialogs.info(f"Your Astral Client ({label}) is up to date.")
        return
    new = release["version"].removeprefix("client-v")
    if release.get("full_download_required"):
        text = (f"Astral Client {new} includes new game data or libraries and must be downloaded in full.\n\n"
                f"Extract it to a new folder, then copy your save and config folders across.\n\n{release['url']}")
        if dialogs.question(text + "\n\nOpen the download page?", "Open page", "Close") and not open_page(release["url"]):
            dialogs.info(release["url"])
        return
    staged = state / f"release-{release['asset']['id']}.zip"
    action = "Install" if staged.is_file() else "Download and install"
    if not dialogs.question(f"{action} Astral Client {new}? You have {label}.\n\n"
                            "Saves, settings, mods and tilesets are preserved, and the current version is backed up for rollback.",
                            "Update", "Not now"):
        return
    try:
        with dialogs.progress(f"Downloading Astral Client {new}…") as report:
            package = download(repo, release, state, report)
    except Cancelled:
        return
    if running(client):
        dialogs.info(f"Astral Client {new} is downloaded and verified.\n\n"
                     "CDDA is still running. Save and quit, then run the updater again to install it — it will not download again.")
        return
    result = apply(package, client, state)
    prune(state)
    dialogs.info(f"Installed Astral Client {result['version'].removeprefix('client-v')}.\n\n"
                 "Your previous version is backed up; use Rollback Astral Client if something is wrong.")

def gui_rollback(client, state, dialogs):
    marker = state / "installed.json"
    if not marker.exists():
        raise UpdateError("There is no updater backup to roll back to.")
    info = json.loads(marker.read_text())
    record = json.loads((Path(info["backup"]) / "backup.json").read_text())
    previous = (record.get("previous_install") or {}).get("version") or "the version you had before updating"
    if not dialogs.question(f"Replace Astral Client {info['version'].removeprefix('client-v')} with {previous.removeprefix('client-v')}?\n\n"
                            "Saves, settings, mods and tilesets are not changed.", "Roll back", "Cancel"):
        return
    rollback(client, state)
    prune(state)
    dialogs.info("The previous Astral Client was restored.")

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=["check", "status", "download", "apply", "rollback", "gui", "gui-rollback"])
    parser.add_argument("--client", type=Path, default=Path(__file__).resolve().parents[2] / "artifacts/client")
    parser.add_argument("--state", type=Path)
    parser.add_argument("--package", type=Path)
    parser.add_argument("--repo", default=REPOSITORY)
    args = parser.parse_args()
    client = args.client.resolve()
    state = (args.state or default_state(client)).resolve()
    if state == client or client in state.parents:
        parser.error("Updater state must be outside the client installation")
    dialogs = None
    try:
        if args.command == "check":
            print(json.dumps(check(args.repo), indent=2))
        elif args.command == "status":
            release = check(args.repo)
            current = installed_version(client, state)
            print(json.dumps({"installed": current, "latest": release["version"], "state": str(state),
                              "up_to_date": bool(current and is_current(current, release["version"])),
                              "full_download_required": bool(release.get("full_download_required"))}, indent=2))
        elif args.command == "download":
            print(download(args.repo, check(args.repo), state))
        elif args.command == "apply":
            if not args.package:
                parser.error("apply requires --package")
            result = apply(args.package, client, state)
            prune(state, keep=args.package)
            print(json.dumps(result, indent=2))
        elif args.command == "rollback":
            rollback(client, state)
            prune(state)
            print("Previous client restored.")
        else:
            dialogs = Dialogs()
            if args.command == "gui":
                gui_update(args.repo, client, state, dialogs)
            else:
                gui_rollback(client, state, dialogs)
    except KeyboardInterrupt:
        return 130
    except (UpdateError, OSError, ValueError, KeyError, zipfile.BadZipFile, TypeError, subprocess.SubprocessError) as error:
        message = str(error)
        if dialogs and dialogs.kind != "terminal":
            dialogs.error(message)
        print(message, file=sys.stderr)
        return 1
    return 0

if __name__ == "__main__":
    sys.exit(main())
