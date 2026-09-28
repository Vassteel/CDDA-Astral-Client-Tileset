param([string]$ReleaseTag, [string]$Output)
$ErrorActionPreference = 'Stop'
if ($ReleaseTag -notmatch '^client-v[0-9.]+$') { throw 'Invalid release tag' }
[IO.Directory]::CreateDirectory($Output) | Out-Null
$release = Invoke-RestMethod "https://api.github.com/repos/Vassteel/CDDA-Astral-Client-Tileset/releases/tags/$ReleaseTag"
function Download-Asset([string]$Suffix) {
    $assets = @($release.assets | Where-Object { $_.name.EndsWith($Suffix) })
    if ($assets.Count -ne 1) { throw "Expected one $Suffix asset" }
    $asset = $assets[0]
    $path = Join-Path $Output $asset.name
    Invoke-WebRequest -UseBasicParsing $asset.browser_download_url -OutFile $path
    if ((Get-Item $path).Length -ne $asset.size) { throw 'Incomplete archive' }
    if (('sha256:' + (Get-FileHash $path).Hash.ToLowerInvariant()) -cne $asset.digest) { throw 'Archive checksum mismatch' }
    return $path
}
$full = Download-Asset '-windows-x64.zip'
$update = Download-Asset '-windows-update.zip'
Expand-Archive $full (Join-Path $Output 'unpacked')
$client = (Get-ChildItem (Join-Path $Output 'unpacked') -Directory | Select-Object -First 1).FullName
$exe = Join-Path $client 'cataclysm-tiles.exe'
$profile = Join-Path $Output 'profile'
[IO.Directory]::CreateDirectory($profile) | Out-Null
$profileArgument = '"' + $profile.Replace('\', '/') + '/"'
$env:SDL_AUDIODRIVER = 'dummy'
$env:SDL_VIDEO_DRIVER = 'windows'
$env:SDL_VIDEODRIVER = 'windows'
$p = Start-Process $exe -WorkingDirectory $client -ArgumentList @('--basepath', '""', '--userdir', $profileArgument, '--check-mods', 'dda') -RedirectStandardOutput (Join-Path $Output 'data-check.log') -RedirectStandardError (Join-Path $Output 'data-check-errors.log') -PassThru
if (!$p.WaitForExit(180000)) { $p.Kill(); throw 'Core data check timed out' }
if ($p.ExitCode -ne 0) { throw "Core data check failed: $($p.ExitCode)" }
. (Join-Path $client 'tools/hybrid-updater/windows-updater.ps1')
$state = Join-Path $Output 'updater-state'
[IO.Directory]::CreateDirectory($state) | Out-Null
$p = Start-Process $exe -WorkingDirectory $client -ArgumentList @('--basepath', '""', '--userdir', $profileArgument) -RedirectStandardOutput (Join-Path $Output 'menu.log') -RedirectStandardError (Join-Path $Output 'menu-errors.log') -PassThru
try {
    Start-Sleep -Seconds 15
    $p.Refresh()
    if ($p.HasExited -or !$p.MainWindowHandle -or $p.MainWindowTitle -notlike '*Astral Client*') { throw 'Game window did not open' }
    $title = $p.MainWindowTitle
    Add-Type -AssemblyName System.Drawing
    Add-Type -AssemblyName System.Windows.Forms
    $screen = [Windows.Forms.Screen]::PrimaryScreen.Bounds
    $image = New-Object Drawing.Bitmap($screen.Width, $screen.Height)
    $graphics = [Drawing.Graphics]::FromImage($image)
    try {
        $graphics.CopyFromScreen($screen.Location, [Drawing.Point]::Empty, $screen.Size)
        $image.Save((Join-Path $Output 'menu.png'), [Drawing.Imaging.ImageFormat]::Png)
    } finally { $graphics.Dispose(); $image.Dispose() }
    $blocked = $false
    try { Invoke-Install $update $client $state | Out-Null }
    catch { if ($_.Exception.Message -like 'Close Astral Client*') { $blocked = $true } else { throw } }
    if (!$blocked) { throw 'Running-game guard failed' }
} finally {
    if (!$p.HasExited) { $p.CloseMainWindow() | Out-Null; if (!$p.WaitForExit(10000)) { $p.Kill(); $p.WaitForExit() } }
}
$before = Get-FileDigest $exe
Invoke-Install $update $client $state
if ((Get-InstalledVersion $client $state) -ne $ReleaseTag) { throw 'Installed version mismatch' }
Invoke-Rollback $client $state
if ((Get-FileDigest $exe) -ne $before) { throw 'Rollback changed original executable' }
@{title=$title; data_check='passed'; process_guard='passed'; update='passed'; rollback='passed'; powershell=$PSVersionTable.PSVersion.ToString()} | ConvertTo-Json | Set-Content (Join-Path $Output 'result.json')
