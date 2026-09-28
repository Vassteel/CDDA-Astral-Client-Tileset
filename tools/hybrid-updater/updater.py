#!/usr/bin/env python3
"""Verified, reversible Linux updates for the Hybrid client (standard library only)."""
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
import urllib.request
import zipfile

LIMIT = 512 * 1024 * 1024
REPOSITORY = "Vassteel/CDDA-Astral-Client-Tileset"

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
    temporary.write_text(json.dumps(value, indent=2) + "\n")
    os.replace(temporary, path)

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

def github_json(repo, suffix):
    gh = gh_binary()
    endpoint = f"repos/{repo}/{suffix}"
    if gh:
        result = subprocess.run([gh, "api", endpoint], capture_output=True, text=True, timeout=60)
        if result.returncode:
            raise UpdateError("GitHub request failed. Check your gh login and repository access.")
        return json.loads(result.stdout)
    with urllib.request.urlopen("https://api.github.com/" + endpoint, timeout=60) as response:
        return json.load(response)

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

def download(repo, release, state):
    if release.get("full_download_required"):
        raise UpdateError(f"{release['version']} requires the full client download: {release['url']}")
    state.mkdir(parents=True, exist_ok=True)
    asset = release["asset"]
    # The asset ID comes from authenticated GitHub metadata, never a local path.
    if not isinstance(asset["id"], int):
        raise UpdateError("Invalid release asset")
    target = state / f"release-{asset['id']}.zip"
    temporary = target.with_suffix(".download")
    gh = gh_binary()
    try:
        with open(temporary, "wb") as out:
            if gh:
                result = subprocess.run([gh, "api", "-H", "Accept: application/octet-stream", f"repos/{repo}/releases/assets/{asset['id']}"], stdout=out, stderr=subprocess.PIPE, timeout=600)
                if result.returncode:
                    raise UpdateError("Release download failed")
            else:
                url = asset["browser_download_url"]
                if not url.startswith(f"https://github.com/{repo}/releases/download/"):
                    raise UpdateError("Unexpected release download URL")
                with urllib.request.urlopen(url, timeout=60) as source:
                    total = 0
                    while chunk := source.read(1024 * 1024):
                        total += len(chunk)
                        if total > LIMIT:
                            raise UpdateError("Download exceeds size limit")
                        out.write(chunk)
        if temporary.stat().st_size != asset["size"]:
            raise UpdateError("Incomplete release download")
        expected = asset.get("digest")
        if expected and expected.startswith("sha256:") and digest(temporary) != expected[7:]:
            raise UpdateError("Release archive checksum mismatch")
        manifest = inspect_package(temporary)
        if manifest["version"] != release["version"]:
            raise UpdateError("Release/package version mismatch")
        os.replace(temporary, target)
        return target
    finally:
        temporary.unlink(missing_ok=True)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=["check", "download", "apply", "rollback", "gui"])
    parser.add_argument("--client", type=Path, default=Path(__file__).resolve().parents[2] / "artifacts/client")
    parser.add_argument("--state", type=Path)
    parser.add_argument("--package", type=Path)
    parser.add_argument("--repo", default=REPOSITORY)
    args = parser.parse_args()
    client = args.client.resolve()
    state = (args.state or client.parent / "hybrid-updates").resolve()
    if state == client or client in state.parents:
        parser.error("Updater state must be outside the client installation")
    try:
        if args.command == "check":
            print(json.dumps(check(args.repo), indent=2))
        elif args.command == "download":
            print(download(args.repo, check(args.repo), state))
        elif args.command == "apply":
            if not args.package:
                parser.error("apply requires --package")
            print(json.dumps(apply(args.package, client, state), indent=2))
        elif args.command == "rollback":
            rollback(client, state)
            print("Previous client restored.")
        else:
            if not shutil.which("zenity"):
                raise UpdateError("Graphical updater requires zenity; CLI commands remain available")
            release = check(args.repo)
            version_file = client / "VERSION.json"
            if version_file.is_file():
                info = json.loads(version_file.read_text())
                if ("client-v" + info.get("version", "") == release["version"] and
                        info.get("executable_sha256") == digest(client / "cataclysm-tiles")):
                    subprocess.run(["zenity", "--info", "--title=Astral Client Update", "--text", "Your Astral Client is up to date."])
                    return 0
            if release.get("full_download_required"):
                raise UpdateError(f"{release['version']} includes new game data. Download the full client from {release['url']}")
            marker = state / "installed.json"
            if marker.exists():
                installed = json.loads(marker.read_text())
                if installed.get("version") == release["version"] and installed.get("client") == str(client) and all(destination(client, f["path"]).is_file() and digest(destination(client, f["path"])) == f["sha256"] for f in installed.get("files", [])):
                    subprocess.run(["zenity", "--info", "--title=Astral Client Update", "--text", "Your Astral Client is up to date."])
                    return 0
            answer = subprocess.run(["zenity", "--question", "--title=Astral Client Update", "--text", f"Download and install {release['version']}?\n\nSaves and settings are preserved. If CDDA is running, the download will be staged until you exit."])
            if answer.returncode:
                return 0
            package = download(args.repo, release, state)
            result = apply(package, client, state)
            subprocess.run(["zenity", "--info", "--title=Astral Client Update", "--text", f"Installed {result['version']}. Your previous client is backed up.\nLaunch CDDA when ready."])
    except (UpdateError, OSError, ValueError, KeyError, zipfile.BadZipFile, TypeError, subprocess.SubprocessError) as error:
        message = str(error)
        if args.command == "gui" and shutil.which("zenity"):
            subprocess.run(["zenity", "--error", "--title=Astral Client Update", "--text", message])
        print(message, file=sys.stderr)
        return 1
    return 0

if __name__ == "__main__":
    sys.exit(main())
