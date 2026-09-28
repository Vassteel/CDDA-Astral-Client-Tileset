[CmdletBinding()]
param(
    [ValidateSet('Gui','Check','Download','Apply','Rollback')][string]$Action = 'Gui',
    [string]$ClientDirectory = (Split-Path (Split-Path $PSScriptRoot -Parent) -Parent),
    [string]$StateDirectory = '',
    [string]$Package = '',
    [string]$Repository = 'Vassteel/CDDA-Astral-Client-Tileset'
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

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
    } finally { Remove-Item -LiteralPath $rescue -Recurse -Force }
    return 'Previous client restored. Saves and settings were preserved.'
}
function Get-Release([string]$Repo) {
    if ($Repo -notmatch '^[\w.-]+/[\w.-]+$') { throw 'Invalid repository' }
    $releases = Invoke-RestMethod -Uri "https://api.github.com/repos/$Repo/releases?per_page=100" -Headers @{'User-Agent'='Astral-Client-Updater'}
    foreach ($release in $releases) {
        if ($release.draft -or $release.prerelease -or $release.tag_name -notmatch '^client-v') { continue }
        $assets = @($release.assets | Where-Object { $_.name.EndsWith('-windows-update.zip') })
        if ($assets.Count -eq 1) { return @{version=$release.tag_name; asset=$assets[0]} }
    }
    throw 'No Windows client update is available'
}
function Save-Download($Release, [string]$State) {
    $asset = $Release.asset
    if ($asset.size -lt 1 -or $asset.size -gt 536870912 -or $asset.id -notmatch '^\d+$') { throw 'Invalid download metadata' }
    if ($asset.browser_download_url -notmatch '^https://github\.com/') { throw 'Invalid download URL' }
    $path = Join-Path $State ("release-$($asset.id).zip")
    $temp = "$path.part"
    try {
        Invoke-WebRequest -UseBasicParsing -Uri $asset.browser_download_url -OutFile $temp -Headers @{'User-Agent'='Astral-Client-Updater'}
        if ((Get-Item -LiteralPath $temp).Length -ne $asset.size) { throw 'Incomplete download' }
        if ($asset.digest -and $asset.digest -cne ('sha256:' + (Get-FileDigest $temp))) { throw 'Download checksum mismatch' }
        Move-Item -LiteralPath $temp -Destination $path -Force
        return $path
    } finally { if (Test-Path -LiteralPath $temp) { Remove-Item -LiteralPath $temp -Force } }
}
function Get-InstalledVersion([string]$Client, [string]$State) {
    $record = Join-Path $State 'installed.json'
    if (Test-Path -LiteralPath $record) {
        $info = Get-Content -Raw -LiteralPath $record | ConvertFrom-Json
        if ($info.client -ine $Client) { throw 'Updater state belongs to another installation' }
        foreach ($file in $info.files) {
            if ((Get-FileDigest (Get-Target $Client $file.path)) -ine $file.sha256) { return '' }
        }
        return $info.version
    }
    $record = Join-Path $Client 'VERSION.json'
    if (Test-Path -LiteralPath $record) {
        $info = Get-Content -Raw -LiteralPath $record | ConvertFrom-Json
        if ($info.executable_sha256 -and (Get-FileDigest (Get-Target $Client 'cataclysm-tiles.exe')) -ieq $info.executable_sha256) {
            return ('client-v' + $info.version)
        }
    }
    return ''
}
function Invoke-Updater {
    $client = [IO.Path]::GetFullPath($ClientDirectory).TrimEnd([IO.Path]::DirectorySeparatorChar)
    $state = $StateDirectory
    if (!$state) { $state = Join-Path (Split-Path $client -Parent) ('.' + (Split-Path $client -Leaf) + '-updates') }
    $state = [IO.Path]::GetFullPath($state)
    if ($state -ieq $client -or $state.StartsWith($client + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { throw 'Updater state must be outside the client folder' }
    [IO.Directory]::CreateDirectory($state) | Out-Null
    $lock = [IO.File]::Open((Join-Path $state 'updater.lock'), [IO.FileMode]::OpenOrCreate, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None)
    try {
        switch ($Action) {
            'Check' { Get-Release $Repository | ConvertTo-Json -Depth 12; return }
            'Download' { Save-Download (Get-Release $Repository) $state; return }
            'Apply' { Invoke-Install ([IO.Path]::GetFullPath($Package)) $client $state; return }
            'Rollback' { Invoke-Rollback $client $state; return }
        }
        Add-Type -AssemblyName System.Windows.Forms
        $release = Get-Release $Repository
        $installedVersion = Get-InstalledVersion $client $state
        if ($installedVersion -eq $release.version) {
            [Windows.Forms.MessageBox]::Show('Astral Client is up to date.', 'Astral Client', 'OK', 'Information') | Out-Null
            return
        }
        $answer = [Windows.Forms.MessageBox]::Show("Download and install $($release.version)?`n`nClose the game before installation. Your saves and settings are preserved.", 'Astral Client', 'YesNo', 'Information')
        if ($answer -ne 'Yes') { return }
        $zip = Save-Download $release $state
        $message = Invoke-Install $zip $client $state
        [Windows.Forms.MessageBox]::Show($message, 'Astral Client', 'OK', 'Information') | Out-Null
    } finally { $lock.Dispose() }
}
if ($MyInvocation.InvocationName -ne '.') {
    try { Invoke-Updater }
    catch {
        if ($Action -eq 'Gui') {
            Add-Type -AssemblyName System.Windows.Forms
            [Windows.Forms.MessageBox]::Show($_.Exception.Message, 'Astral Client', 'OK', 'Error') | Out-Null
        } else { Write-Error $_ }
        exit 1
    }
}
