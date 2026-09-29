[CmdletBinding()]
param(
    [ValidateSet('Gui','GuiRollback','Check','Status','Download','Apply','Rollback')][string]$Action = 'Gui',
    [string]$ClientDirectory = (Split-Path (Split-Path $PSScriptRoot -Parent) -Parent),
    [string]$StateDirectory = '',
    [string]$Package = '',
    [string]$Repository = 'Vassteel/CDDA-Astral-Client-Tileset'
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
# Windows PowerShell 5.1 redraws its progress bar per packet, which makes
# Invoke-WebRequest/RestMethod many times slower. The GUI shows its own.
$ProgressPreference = 'SilentlyContinue'
$script:UserAgent = 'Astral-Client-Updater'
$script:Retries = 3

function ConvertTo-ClientVersion([string]$Tag) {
    if ($Tag -match '^client-v(\d+(\.\d+){0,3})$') {
        $parts = @($Matches[1].Split('.') | ForEach-Object { [int]$_ })
        while ($parts.Count -lt 4) { $parts += 0 }
        return New-Object Version($parts[0], $parts[1], $parts[2], $parts[3])
    }
    return $null
}
function Test-UpToDate([string]$Installed, [string]$Latest) {
    # Never offer a downgrade: a newer local build counts as up to date.
    if (!$Installed) { return $false }
    $have = ConvertTo-ClientVersion $Installed
    $want = ConvertTo-ClientVersion $Latest
    if ($have -and $want) { return $have -ge $want }
    return $Installed -eq $Latest
}
function Invoke-WithRetry([scriptblock]$Run) {
    for ($attempt = 1; ; $attempt++) {
        try { return & $Run }
        catch {
            # Only network failures are retried; validation errors and cancellation are final.
            $e = $_.Exception
            # .NET calls such as GetResponse() arrive wrapped by PowerShell.
            while ($e -is [Management.Automation.MethodInvocationException] -and $e.InnerException) { $e = $e.InnerException }
            if (!($e -is [Net.WebException] -or $e -is [IO.IOException] -or $e.GetType().Name -match '^Http')) { throw }
            $status = $null
            if ($e.Response) { try { $status = [int]$e.Response.StatusCode } catch { } }
            if ($status -eq 403) { throw "GitHub refused the request (HTTP 403). The hourly update-check limit may have been reached; try again later." }
            if (($status -and $status -lt 500) -or $attempt -ge $script:Retries) {
                if ($status) { throw "GitHub returned HTTP $status. Try again later." }
                throw "Could not reach GitHub. Check your internet connection and try again. ($($_.Exception.Message))"
            }
            Start-Sleep -Seconds ([Math]::Pow(2, $attempt - 1))
        }
    }
}
function Get-FileDigest([string]$Path) { (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant() }
function Write-Record($Value, [string]$Path) {
    $temporary = "$Path.new"
    [IO.File]::WriteAllText($temporary, ($Value | ConvertTo-Json -Depth 16), (New-Object Text.UTF8Encoding($false)))
    if ([IO.File]::Exists($Path)) { [IO.File]::Replace($temporary, $Path, [NullString]::Value) }
    else { [IO.File]::Move($temporary, $Path) }
}
function Get-Target([string]$Root, [string]$Relative) {
    if ($Relative -cnotmatch '^(cataclysm-tiles\.exe|data/title/astral\.(png|jpg|jpeg))$') { throw "Unsupported update path: $Relative" }
    $path = [IO.Path]::GetFullPath((Join-Path $Root $Relative))
    $part = $path
    while ($part) {
        if (Test-Path -LiteralPath $part) {
            if ((Get-Item -Force -LiteralPath $part).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Linked update path refused: $part" }
        }
        $part = Split-Path $part -Parent
    }
    return $path
}
function Assert-GameClosed([string]$Client) {
    foreach ($process in @(Get-Process -Name 'cataclysm-tiles' -ErrorAction SilentlyContinue)) {
        try { $path = $process.Path } catch { $path = $null }
        if (!$path -or [IO.Path]::GetFullPath($path) -ieq (Join-Path $Client 'cataclysm-tiles.exe')) {
            throw 'Close Astral Client before installing or rolling back. The download remains staged.'
        }
    }
}
function Read-Package([string]$ZipPath, [string]$ExtractTo) {
    $archive = [IO.Compression.ZipFile]::OpenRead($ZipPath)
    try {
        $entries = New-Object 'Collections.Generic.Dictionary[string,object]' ([StringComparer]::OrdinalIgnoreCase)
        foreach ($entry in $archive.Entries) {
            if ($entries.ContainsKey($entry.FullName)) { throw 'Duplicate archive entry' }
            $entries.Add($entry.FullName, $entry)
            if (($entry.ExternalAttributes -shr 16 -band 0xF000) -eq 0xA000) { throw 'Archive links are not supported' }
        }
        if (!$entries.ContainsKey('update.json') -or $entries['update.json'].Length -gt 1048576) { throw 'Missing or oversized update manifest' }
        $reader = New-Object IO.StreamReader($entries['update.json'].Open())
        try { $manifest = $reader.ReadToEnd() | ConvertFrom-Json } finally { $reader.Dispose() }
        if ($manifest.schema -ne 1 -or $manifest.platform -cne 'windows-x86_64' -or $manifest.version -notmatch '^client-v[0-9]') { throw 'Not an Astral Windows update' }
        $files = @($manifest.files)
        if ($files.Count -lt 1 -or $files.Count -gt 64 -or $entries.Count -ne $files.Count + 1) { throw 'Unexpected archive contents' }
        $seen = New-Object 'Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)
        $total = 0L
        foreach ($file in $files) {
            $target = Get-Target $ExtractTo $file.path
            if (!$seen.Add($file.path)) { throw 'Duplicate manifest path' }
            if ($file.sha256 -notmatch '^[a-fA-F0-9]{64}$' -or $file.size -lt 0 -or $file.size -gt 536870912) { throw 'Invalid file metadata' }
            $total += $file.size
            if ($total -gt 536870912) { throw 'Update is too large' }
            $key = 'payload/' + $file.path
            if (!$entries.ContainsKey($key) -or $entries[$key].Length -ne $file.size) { throw 'Missing file or size mismatch' }
            [IO.Directory]::CreateDirectory((Split-Path $target -Parent)) | Out-Null
            [IO.Compression.ZipFileExtensions]::ExtractToFile($entries[$key], $target, $false)
            if ((Get-FileDigest $target) -cne $file.sha256.ToLowerInvariant()) { throw 'Update checksum mismatch' }
        }
        if (!$seen.Contains('cataclysm-tiles.exe')) { throw 'Update has no Windows executable' }
        return $manifest
    } finally { $archive.Dispose() }
}
function Copy-Atomic([string]$Source, [string]$Target) {
    [IO.Directory]::CreateDirectory((Split-Path $Target -Parent)) | Out-Null
    $temp = $Target + '.' + [guid]::NewGuid().ToString('N') + '.tmp'
    try {
        [IO.File]::Copy($Source, $temp, $false)
        if ([IO.File]::Exists($Target)) { [IO.File]::Replace($temp, $Target, [NullString]::Value) }
        else { [IO.File]::Move($temp, $Target) }
    } finally { if ([IO.File]::Exists($temp)) { [IO.File]::Delete($temp) } }
}
function Restore-Files($Record, [string]$Backup, [string]$Client) {
    foreach ($file in $Record.files) {
        $target = Get-Target $Client $file.path
        if ($file.existed) { Copy-Atomic (Join-Path $Backup ('files/' + $file.path)) $target }
        elseif ([IO.File]::Exists($target)) { [IO.File]::Delete($target) }
    }
}
function Invoke-Install([string]$ZipPath, [string]$Client, [string]$State) {
    if (!(Test-Path -LiteralPath (Join-Path $Client 'cataclysm-tiles.exe') -PathType Leaf)) { throw 'Choose an existing Windows client installation' }
    $verify = Join-Path $State ('verify-' + [guid]::NewGuid().ToString('N'))
    [IO.Directory]::CreateDirectory($verify) | Out-Null
    try {
        $manifest = Read-Package $ZipPath $verify
        Assert-GameClosed $Client
        $backup = Join-Path $State ('backups/' + [guid]::NewGuid().ToString('N'))
        [IO.Directory]::CreateDirectory($backup) | Out-Null
        $installed = Join-Path $State 'installed.json'
        $previous = $null
        if (Test-Path -LiteralPath $installed) {
            $previous = Get-Content -Raw -LiteralPath $installed | ConvertFrom-Json
            if ($previous.client -ine $Client) { throw 'Updater state belongs to another installation' }
        }
        $record = @{ client=$Client; version=$manifest.version; previous=$previous; files=@(); status='prepared' }
        foreach ($file in $manifest.files) {
            $target = Get-Target $Client $file.path
            $entry = @{path=$file.path; existed=[IO.File]::Exists($target); installed_sha256=$file.sha256}
            if ($entry.existed) {
                $entry.sha256 = Get-FileDigest $target
                $saved = Join-Path $backup ('files/' + $file.path)
                [IO.Directory]::CreateDirectory((Split-Path $saved -Parent)) | Out-Null
                [IO.File]::Copy($target, $saved)
            }
            $record.files += $entry
        }
        Write-Record $record (Join-Path $backup 'backup.json')
        Assert-GameClosed $Client
        try {
            foreach ($file in $manifest.files) { Copy-Atomic (Join-Path $verify $file.path) (Get-Target $Client $file.path) }
            Write-Record @{client=$Client; version=$manifest.version; backup=$backup; files=$manifest.files} $installed
        } catch {
            Restore-Files $record $backup $Client
            $record.status = 'failed-restored'
            Write-Record $record (Join-Path $backup 'backup.json')
            throw
        }
        $record.status = 'installed'
        Write-Record $record (Join-Path $backup 'backup.json')
        return "Installed $($manifest.version). Backup saved outside the game folder."
    } finally { Remove-Item -LiteralPath $verify -Recurse -Force }
}
function Invoke-Rollback([string]$Client, [string]$State) {
    $installed = Join-Path $State 'installed.json'
    if (!(Test-Path -LiteralPath $installed)) { throw 'No updater backup is available' }
    $current = Get-Content -Raw -LiteralPath $installed | ConvertFrom-Json
    if ($current.client -ine $Client) { throw 'Backup belongs to another installation' }
    $record = Get-Content -Raw -LiteralPath (Join-Path $current.backup 'backup.json') | ConvertFrom-Json
    if ($record.client -ine $Client) { throw 'Invalid backup installation' }
    foreach ($file in $record.files) {
        if ((Get-FileDigest (Get-Target $Client $file.path)) -cne $file.installed_sha256) { throw 'Installed files changed after updating; rollback cancelled' }
        if ($file.existed -and (Get-FileDigest (Join-Path $current.backup ('files/' + $file.path))) -cne $file.sha256) { throw 'Backup checksum mismatch' }
    }
    Assert-GameClosed $Client
    $rescue = Join-Path $State ('rollback-' + [guid]::NewGuid().ToString('N'))
    [IO.Directory]::CreateDirectory($rescue) | Out-Null
    try {
        foreach ($file in $record.files) {
            $saved = Join-Path $rescue $file.path
            [IO.Directory]::CreateDirectory((Split-Path $saved -Parent)) | Out-Null
            [IO.File]::Copy((Get-Target $Client $file.path), $saved)
        }
        Assert-GameClosed $Client
        try {
            Restore-Files $record $current.backup $Client
            if ($record.previous) { Write-Record $record.previous $installed }
            else { [IO.File]::Delete($installed) }
        } catch {
            foreach ($file in $record.files) { Copy-Atomic (Join-Path $rescue $file.path) (Get-Target $Client $file.path) }
            throw
        }
        $record.status = 'rolled-back'
        Write-Record $record (Join-Path $current.backup 'backup.json')
    } finally { Remove-Item -LiteralPath $rescue -Recurse -Force }
    return 'Previous client restored. Saves and settings were preserved.'
}
function Get-Release([string]$Repo) {
    if ($Repo -notmatch '^[\w.-]+/[\w.-]+$') { throw 'Invalid repository' }
    $releases = Invoke-WithRetry { Invoke-RestMethod -UseBasicParsing -Uri "https://api.github.com/repos/$Repo/releases?per_page=100" -Headers @{'User-Agent'=$script:UserAgent} }
    foreach ($release in $releases) {
        if ($release.draft -or $release.prerelease -or $release.tag_name -notmatch '^client-v') { continue }
        $assets = @($release.assets | Where-Object { $_.name.EndsWith('-windows-update.zip') })
        if ($assets.Count -eq 1) { return @{version=$release.tag_name; url=$release.html_url; asset=$assets[0]} }
        return @{version=$release.tag_name; full_download_required=$true; url=$release.html_url}
    }
    throw 'No Windows client update is available'
}
function Receive-File([string]$Uri, [string]$Path, [long]$Size, [scriptblock]$OnProgress) {
    # Stream on this thread so a progress window can stay responsive via its callback.
    $request = [Net.WebRequest]::Create($Uri)
    $request.UserAgent = $script:UserAgent
    $request.Timeout = 60000
    $request.ReadWriteTimeout = 60000
    $response = $request.GetResponse()
    try {
        $source = $response.GetResponseStream()
        $out = [IO.File]::Create($Path)
        try {
            $buffer = New-Object byte[] 1048576
            $done = 0L
            while (($read = $source.Read($buffer, 0, $buffer.Length)) -gt 0) {
                $done += $read
                if ($done -gt 536870912) { throw 'Download exceeds size limit' }
                $out.Write($buffer, 0, $read)
                if ($OnProgress) { & $OnProgress $done $Size }
            }
        } finally { $out.Dispose(); $source.Dispose() }
    } finally { $response.Dispose() }
}
function Test-Download([string]$Path, $Release) {
    $asset = $Release.asset
    if ((Get-Item -LiteralPath $Path).Length -ne $asset.size) { throw 'Incomplete download' }
    if ($asset.digest -and $asset.digest -cne ('sha256:' + (Get-FileDigest $Path))) { throw 'Download checksum mismatch' }
    $verify = Join-Path (Split-Path $Path -Parent) ('verify-' + [guid]::NewGuid().ToString('N'))
    [IO.Directory]::CreateDirectory($verify) | Out-Null
    try {
        if ((Read-Package $Path $verify).version -cne $Release.version) { throw 'Release/package version mismatch' }
    } finally { Remove-Item -LiteralPath $verify -Recurse -Force }
}
function Save-Download($Release, [string]$State, [string]$Repo = $Repository, [scriptblock]$OnProgress = $null) {
    if ($Release.full_download_required) { throw "$($Release.version) requires the full client download: $($Release.url)" }
    $asset = $Release.asset
    if ($asset.size -lt 1 -or $asset.size -gt 536870912 -or $asset.id -notmatch '^\d+$') { throw 'Invalid download metadata' }
    if (!$asset.browser_download_url.StartsWith("https://github.com/$Repo/releases/download/", [StringComparison]::OrdinalIgnoreCase)) { throw 'Invalid download URL' }
    $path = Join-Path $State ("release-$($asset.id).zip")
    if (Test-Path -LiteralPath $path) {
        # Staged earlier (e.g. while the game was running): no second download.
        try { Test-Download $path $Release; return $path }
        catch { Remove-Item -LiteralPath $path -Force }
    }
    $temp = "$path.part"
    try {
        Invoke-WithRetry { Receive-File $asset.browser_download_url $temp $asset.size $OnProgress } | Out-Null
        Test-Download $temp $Release
        Move-Item -LiteralPath $temp -Destination $path -Force
        return $path
    } finally { if (Test-Path -LiteralPath $temp) { Remove-Item -LiteralPath $temp -Force } }
}
function Get-RollbackChain([string]$State) {
    $chain = New-Object 'Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)
    $marker = Join-Path $State 'installed.json'
    $current = $null
    if (Test-Path -LiteralPath $marker) { $current = Get-Content -Raw -LiteralPath $marker | ConvertFrom-Json }
    while ($current -and $current.backup -and $chain.Add([IO.Path]::GetFullPath([string]$current.backup))) {
        try { $current = (Get-Content -Raw -LiteralPath (Join-Path $current.backup 'backup.json') | ConvertFrom-Json).previous }
        catch { break }
    }
    return ,$chain
}
function Remove-StaleUpdates([string]$State, [string]$Keep = '') {
    # Finished downloads and backups rollback can no longer reach. Interrupted
    # ('prepared') backups stay for manual recovery.
    foreach ($staged in @(Get-ChildItem -LiteralPath $State -Filter 'release-*' -File -ErrorAction SilentlyContinue)) {
        if (!$Keep -or $staged.FullName -ine [IO.Path]::GetFullPath($Keep)) { Remove-Item -LiteralPath $staged.FullName -Force }
    }
    $chain = Get-RollbackChain $State
    foreach ($backup in @(Get-ChildItem -LiteralPath (Join-Path $State 'backups') -Directory -ErrorAction SilentlyContinue)) {
        try { $status = (Get-Content -Raw -LiteralPath (Join-Path $backup.FullName 'backup.json') | ConvertFrom-Json).status } catch { continue }
        if (($status -eq 'rolled-back' -or $status -eq 'failed-restored') -and !$chain.Contains($backup.FullName)) {
            Remove-Item -LiteralPath $backup.FullName -Recurse -Force
        }
    }
}
function Test-GameRunning([string]$Client) {
    try { Assert-GameClosed $Client; return $false } catch { return $true }
}
function Get-InstalledVersion([string]$Client, [string]$State) {
    $record = Join-Path $State 'installed.json'
    if (Test-Path -LiteralPath $record) {
        $info = Get-Content -Raw -LiteralPath $record | ConvertFrom-Json
        if ($info.client -ine $Client) { throw 'Updater state belongs to another installation' }
        foreach ($file in $info.files) {
            $target = Get-Target $Client $file.path
            if (![IO.File]::Exists($target) -or (Get-FileDigest $target) -ine $file.sha256) { return '' }
        }
        return $info.version
    }
    $record = Join-Path $Client 'VERSION.json'
    if (Test-Path -LiteralPath $record) {
        $info = Get-Content -Raw -LiteralPath $record | ConvertFrom-Json
        if ($info.executable_sha256 -and [IO.File]::Exists((Join-Path $Client 'cataclysm-tiles.exe')) -and (Get-FileDigest (Get-Target $Client 'cataclysm-tiles.exe')) -ieq $info.executable_sha256) {
            return ('client-v' + $info.version)
        }
    }
    return ''
}
function Show-Message([string]$Text, [string]$Buttons = 'OK', [string]$Icon = 'Information') {
    return [string][Windows.Forms.MessageBox]::Show($Text, 'Astral Client', $Buttons, $Icon)
}
function Save-DownloadWithProgress($Release, [string]$State, [string]$Repo) {
    $form = New-Object Windows.Forms.Form
    $form.Text = 'Astral Client'
    $form.FormBorderStyle = 'FixedDialog'
    $form.MaximizeBox = $false; $form.MinimizeBox = $false
    $form.StartPosition = 'CenterScreen'
    $form.ClientSize = New-Object Drawing.Size(420, 110)
    $label = New-Object Windows.Forms.Label
    $label.SetBounds(12, 12, 396, 20)
    $label.Text = "Downloading Astral Client $($Release.version.Substring(8))..."
    $bar = New-Object Windows.Forms.ProgressBar
    $bar.SetBounds(12, 38, 396, 22)
    $cancel = New-Object Windows.Forms.Button
    $cancel.SetBounds(318, 72, 90, 26)
    $cancel.Text = 'Cancel'
    # Not $state: PowerShell names are case-insensitive and $State is a parameter.
    $ui = @{ cancelled = $false }
    $cancel.Add_Click({ $ui.cancelled = $true }.GetNewClosure())
    $form.Add_FormClosing({ $ui.cancelled = $true }.GetNewClosure())
    $form.Controls.AddRange(@($label, $bar, $cancel))
    $form.Show()
    try {
        $progress = {
            param($Done, $Total)
            $bar.Value = [int][Math]::Min(100, $Done * 100 / [Math]::Max($Total, 1))
            $label.Text = 'Downloading Astral Client {0}... {1:N0} / {2:N0} MB' -f $Release.version.Substring(8), ($Done / 1MB), ($Total / 1MB)
            [Windows.Forms.Application]::DoEvents()
            if ($ui.cancelled) { throw 'Download cancelled' }
        }.GetNewClosure()
        [Windows.Forms.Application]::DoEvents()
        try { return Save-Download $Release $State $Repo $progress }
        catch { if ($ui.cancelled) { return $null }; throw }
    } finally { $form.Dispose() }
}
function Invoke-GuiUpdate([string]$Client, [string]$State) {
    $release = Get-Release $Repository
    $installedVersion = Get-InstalledVersion $Client $State
    $label = if ($installedVersion) { $installedVersion.Substring(8) } else { 'an unrecognized version' }
    $new = $release.version.Substring(8)
    if (Test-UpToDate $installedVersion $release.version) {
        Show-Message "Your Astral Client ($label) is up to date." | Out-Null
        return
    }
    if ($release.full_download_required) {
        $answer = Show-Message "Astral Client $new includes new game data or libraries and must be downloaded in full.`n`nExtract it to a new folder, then copy your save and config folders across.`n`nOpen the download page?" 'YesNo'
        if ($answer -eq 'Yes' -and $release.url -match '^https://github\.com/') { Start-Process $release.url }
        return
    }
    $action = if (Test-Path -LiteralPath (Join-Path $State "release-$($release.asset.id).zip")) { 'Install' } else { 'Download and install' }
    $answer = Show-Message "$action Astral Client $($new)? You have $label.`n`nSaves, settings, mods and tilesets are preserved, and the current version is backed up for rollback." 'YesNo'
    if ($answer -ne 'Yes') { return }
    $zip = Save-DownloadWithProgress $release $State $Repository
    if (!$zip) { return }
    if (Test-GameRunning $Client) {
        Show-Message "Astral Client $new is downloaded and verified.`n`nThe game is still running. Save and quit, then run Update Astral Client again to install it. It will not download again." | Out-Null
        return
    }
    Invoke-Install $zip $Client $State | Out-Null
    Remove-StaleUpdates $State
    Show-Message "Installed Astral Client $new.`n`nYour previous version is backed up; use Rollback Astral Client if something is wrong." | Out-Null
}
function Invoke-GuiRollback([string]$Client, [string]$State) {
    $installed = Join-Path $State 'installed.json'
    if (!(Test-Path -LiteralPath $installed)) { throw 'There is no updater backup to roll back to.' }
    $current = Get-Content -Raw -LiteralPath $installed | ConvertFrom-Json
    $record = Get-Content -Raw -LiteralPath (Join-Path $current.backup 'backup.json') | ConvertFrom-Json
    $previous = if ($record.previous -and $record.previous.version) { $record.previous.version.Substring(8) } else { 'the version you had before updating' }
    $answer = Show-Message "Replace Astral Client $($current.version.Substring(8)) with $($previous)?`n`nSaves, settings, mods and tilesets are not changed." 'YesNo' 'Question'
    if ($answer -ne 'Yes') { return }
    Invoke-Rollback $Client $State | Out-Null
    Remove-StaleUpdates $State
    Show-Message 'The previous Astral Client was restored.' | Out-Null
}
function Invoke-Updater {
    $client = [IO.Path]::GetFullPath($ClientDirectory).TrimEnd([IO.Path]::DirectorySeparatorChar)
    $state = $StateDirectory
    if (!$state) { $state = Join-Path (Split-Path $client -Parent) ('.' + (Split-Path $client -Leaf) + '-updates') }
    $state = [IO.Path]::GetFullPath($state)
    if ($state -ieq $client -or $state.StartsWith($client + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { throw 'Updater state must be outside the client folder' }
    [IO.Directory]::CreateDirectory($state) | Out-Null
    try { $lock = [IO.File]::Open((Join-Path $state 'updater.lock'), [IO.FileMode]::OpenOrCreate, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None) }
    catch [IO.IOException] { throw 'Another Astral Client updater is already running.' }
    try {
        switch ($Action) {
            'Check' { Get-Release $Repository | ConvertTo-Json -Depth 12; return }
            'Status' {
                $release = Get-Release $Repository
                $installedVersion = Get-InstalledVersion $client $state
                @{installed=$installedVersion; latest=$release.version; state=$state; up_to_date=(Test-UpToDate $installedVersion $release.version); full_download_required=[bool]$release.full_download_required} | ConvertTo-Json
                return
            }
            'Download' { Save-Download (Get-Release $Repository) $state $Repository; return }
            'Apply' { Invoke-Install ([IO.Path]::GetFullPath($Package)) $client $state; Remove-StaleUpdates $state $Package; return }
            'Rollback' { Invoke-Rollback $client $state; Remove-StaleUpdates $state; return }
        }
        Add-Type -AssemblyName System.Windows.Forms
        Add-Type -AssemblyName System.Drawing
        [Windows.Forms.Application]::EnableVisualStyles()
        if ($Action -eq 'GuiRollback') { Invoke-GuiRollback $client $state }
        else { Invoke-GuiUpdate $client $state }
    } finally { $lock.Dispose() }
}
if ($MyInvocation.InvocationName -ne '.') {
    try { Invoke-Updater }
    catch {
        if ($Action -like 'Gui*') {
            Add-Type -AssemblyName System.Windows.Forms
            [Windows.Forms.MessageBox]::Show($_.Exception.Message, 'Astral Client', 'OK', 'Error') | Out-Null
        } else { Write-Error $_ }
        exit 1
    }
}
