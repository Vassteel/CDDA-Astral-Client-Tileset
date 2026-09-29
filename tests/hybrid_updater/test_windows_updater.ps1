param([string]$Fixtures, [string]$Updater)
$ErrorActionPreference = 'Stop'
. $Updater
$root = Join-Path ([IO.Path]::GetTempPath()) ('astral-updater-test-' + [guid]::NewGuid().ToString('N'))
$client = Join-Path $root 'client'
$state = Join-Path $root 'state'
[IO.Directory]::CreateDirectory($client) | Out-Null
[IO.Directory]::CreateDirectory($state) | Out-Null
$exe = Join-Path $client 'cataclysm-tiles.exe'
[IO.File]::WriteAllText($exe, 'old client')
foreach ($dir in @('save','config','gfx/Astral','data/mods/keep')) {
    [IO.Directory]::CreateDirectory((Join-Path $client $dir)) | Out-Null
    [IO.File]::WriteAllText((Join-Path $client "$dir/keep.txt"), $dir)
}
$script:processes = @()
function Get-Process { param($Name,$ErrorAction) $script:processes }
$script:passed = 0
function Assert($Condition,[string]$Name) {
    if (!$Condition) { throw "FAIL: $Name" }
    $script:passed++; Write-Host "PASS: $Name"
}
function Must-Fail([scriptblock]$Run,[string]$Name) {
    $failed=$false
    try { & $Run | Out-Null } catch { $failed=$true }
    Assert $failed $Name
}
try {
    function Invoke-RestMethod {
        @(@{tag_name='client-v0.1.2'; html_url='https://example.test/new'; assets=@()},
          @{tag_name='client-v0.1.1'; html_url='https://example.test/old'; assets=@(@{name='old-windows-update.zip'})})
    }
    $fullRelease = Get-Release 'owner/repo'
    Assert ($fullRelease.version -eq 'client-v0.1.2') 'full release does not select older update'
    Assert $fullRelease.full_download_required 'full client requirement returned'
    Must-Fail { Save-Download $fullRelease $state } 'full release cannot install old executable'
    $valid=Join-Path $Fixtures 'valid.zip'
    Write-Record @{version='0.1.0';executable_sha256=(Get-FileDigest $exe)} (Join-Path $client 'VERSION.json')
    Assert ((Get-InstalledVersion $client $state) -eq 'client-v0.1.0') 'recognize full distribution version'
    Invoke-Install $valid $client $state | Out-Null
    Assert (([IO.File]::ReadAllText($exe)) -eq 'new client') 'valid install'
    Assert ((Get-InstalledVersion $client $state) -eq 'client-v0.1.1') 'recognize updated version'
    foreach ($dir in @('save','config','gfx/Astral','data/mods/keep')) {
        Assert (([IO.File]::ReadAllText((Join-Path $client "$dir/keep.txt"))) -eq $dir) "preserve $dir"
    }
    Invoke-Rollback $client $state | Out-Null
    Assert ((Get-InstalledVersion $client $state) -eq 'client-v0.1.0') 'recognize restored version'
    Assert (([IO.File]::ReadAllText($exe)) -eq 'old client') 'restore original executable'
    Assert (!(Test-Path (Join-Path $client 'data/title/astral.png'))) 'rollback removes newly installed title'
    foreach ($name in @('corrupt','traversal','extra','duplicate','linux','symlink','missing-exe','size','case-collision')) {
        Must-Fail { Invoke-Install (Join-Path $Fixtures "$name.zip") $client $state } "reject $name"
        Assert (([IO.File]::ReadAllText($exe)) -eq 'old client') "unchanged after $name"
    }
    $script:processes=@(@{Path=$exe})
    Must-Fail { Invoke-Install $valid $client $state } 'running client blocks update'
    $script:processes=@(@{Path=$null})
    Must-Fail { Invoke-Install $valid $client $state } 'unreadable game process blocks update'
    $script:processes=@()
    $originalCopy=${function:Copy-Atomic}
    $script:failOnce=$true
    function Copy-Atomic([string]$Source,[string]$Target) {
        if($script:failOnce -and $Target.EndsWith('astral.png')) { $script:failOnce=$false; throw 'simulated file replacement failure' }
        & $originalCopy $Source $Target
    }
    Must-Fail { Invoke-Install $valid $client $state } 'mid-install failure reported'
    Assert (([IO.File]::ReadAllText($exe)) -eq 'old client') 'mid-install failure restores executable'
    ${function:Copy-Atomic}=$originalCopy
    Invoke-Install $valid $client $state | Out-Null
    $script:processes=@(@{Path=$exe})
    Must-Fail { Invoke-Rollback $client $state } 'running client blocks rollback'
    $script:processes=@()
    [IO.File]::WriteAllText($exe,'user modified')
    Assert ((Get-InstalledVersion $client $state) -eq '') 'modified executable does not claim current version'
    Must-Fail { Invoke-Rollback $client $state } 'changed executable blocks rollback'
    [IO.File]::WriteAllText($exe,'new client')
    $info=Get-Content -Raw (Join-Path $state 'installed.json') | ConvertFrom-Json
    [IO.File]::WriteAllText((Join-Path $info.backup 'files/cataclysm-tiles.exe'),'bad backup')
    Must-Fail { Invoke-Rollback $client $state } 'corrupt backup blocks rollback'
    $ClientDirectory=$client; $StateDirectory=$state; $Package=$valid; $Action='Apply'
    $held=[IO.File]::Open((Join-Path $state 'updater.lock'),'OpenOrCreate','ReadWrite','None')
    try { Must-Fail { Invoke-Updater } 'concurrent updater lock' } finally { $held.Dispose() }
    $StateDirectory=Join-Path $client 'updates'
    Must-Fail { Invoke-Updater } 'state cannot live inside installation'
    function Invoke-RestMethod {
        @(@{tag_name='tileset-v99';draft=$false;prerelease=$false;assets=@(@{name='art.zip'})},
          @{tag_name='client-v0.2';draft=$false;prerelease=$false;assets=@(@{name='Astral-linux-update.zip'})},
          @{tag_name='client-v0.1.1';draft=$false;prerelease=$false;assets=@(@{name='Astral-windows-update.zip'})})
    }
    $platformRelease = Get-Release 'Vassteel/CDDA-Astral-Client-Tileset'
    Assert ($platformRelease.version -eq 'client-v0.2') 'skip tileset without downgrading to an older Windows patch'
    Assert $platformRelease.full_download_required 'missing Windows patch requires full client'
    Assert (Test-UpToDate 'client-v0.10.0' 'client-v0.9.9') 'newer local build is not downgraded'
    Assert (Test-UpToDate 'client-v0.1.2' 'client-v0.1.2') 'same version is up to date'
    Assert (!(Test-UpToDate 'client-v0.1.1' 'client-v0.1.2')) 'older version is offered an update'
    Assert (!(Test-UpToDate '' 'client-v0.1.2')) 'unknown version is offered an update'
    $client2 = Join-Path $root 'client2'
    $state2 = Join-Path $root 'state2'
    [IO.Directory]::CreateDirectory($client2) | Out-Null
    [IO.Directory]::CreateDirectory($state2) | Out-Null
    $exe2 = Join-Path $client2 'cataclysm-tiles.exe'
    [IO.File]::WriteAllText($exe2, 'old client')
    Write-Record @{version='0.1.0';executable_sha256=(Get-FileDigest $exe2)} (Join-Path $client2 'VERSION.json')
    $script:served = $valid
    $script:downloads = 0
    function Receive-File([string]$Uri,[string]$Path,[long]$Size,[scriptblock]$OnProgress) {
        $script:downloads++
        Copy-Item -LiteralPath $script:served -Destination $Path
        if ($OnProgress) { & $OnProgress $Size $Size }
    }
    function New-TestRelease([string]$Version = 'client-v0.1.1') {
        @{version=$Version; url='https://github.com/owner/repo/releases/tag/x'; asset=@{id=5; size=(Get-Item -LiteralPath $valid).Length;
          digest=('sha256:' + (Get-FileDigest $valid)); browser_download_url='https://github.com/owner/repo/releases/download/x/Astral-windows-update.zip'}}
    }
    $release = New-TestRelease
    $zip = Save-Download $release $state2 'owner/repo'
    $zip2 = Save-Download $release $state2 'owner/repo'
    Assert ($zip -eq $zip2 -and $script:downloads -eq 1) 'staged download is reused'
    [IO.File]::WriteAllText($zip, 'damaged')
    Save-Download $release $state2 'owner/repo' | Out-Null
    Assert ($script:downloads -eq 2 -and (Get-FileDigest $zip) -eq (Get-FileDigest $valid)) 'damaged staged download is replaced'
    Must-Fail { Save-Download (New-TestRelease 'client-v0.1.1') $state2 'other/repo' } 'download URL pinned to repository'
    Remove-Item -LiteralPath $zip
    Must-Fail { Save-Download (New-TestRelease 'client-v0.2.0') $state2 'owner/repo' } 'package/release version mismatch rejected'
    Assert (@(Get-ChildItem -LiteralPath $state2 -Filter 'release-*').Count -eq 0) 'rejected download leaves nothing staged'
    $script:attempts = 0
    function Start-Sleep { }
    Must-Fail { Invoke-WithRetry { $script:attempts++; throw (New-Object Net.WebException 'offline') } } 'network failure reported'
    Assert ($script:attempts -eq 3) 'network failures retried'
    $script:attempts = 0
    Must-Fail { Invoke-WithRetry { $script:attempts++; throw 'Download cancelled' } } 'cancellation reported'
    Assert ($script:attempts -eq 1) 'cancellation not retried'
    $script:messages = @()
    $script:answer = 'Yes'
    function Show-Message([string]$Text,[string]$Buttons='OK',[string]$Icon='Information') { $script:messages += $Text; if ($Buttons -eq 'YesNo') { return $script:answer } return 'OK' }
    function Save-DownloadWithProgress($Release,[string]$State,[string]$Repo) { Save-Download $Release $State 'owner/repo' }
    function Get-Release { $script:currentRelease }
    $script:currentRelease = New-TestRelease
    $script:processes = @(@{Path=$exe2})
    $before = $script:downloads
    Invoke-GuiUpdate $client2 $state2
    Assert ($script:messages[-1] -like '*still running*' -and ([IO.File]::ReadAllText($exe2)) -eq 'old client') 'running game stages the download'
    $script:processes = @()
    Invoke-GuiUpdate $client2 $state2
    Assert ($script:messages[-2] -like 'Install Astral Client 0.1.1*' -and $script:downloads -eq $before + 1) 'staged download installs without downloading again'
    Assert ($script:messages[-1] -like 'Installed Astral Client 0.1.1*' -and ([IO.File]::ReadAllText($exe2)) -eq 'new client') 'GUI install'
    Assert (@(Get-ChildItem -LiteralPath $state2 -Filter 'release-*').Count -eq 0) 'installed download removed'
    Invoke-GuiUpdate $client2 $state2
    Assert ($script:messages[-1] -like '*0.1.1) is up to date*') 'GUI reports up to date'
    $script:currentRelease = New-TestRelease 'client-v0.1.0'
    Invoke-GuiUpdate $client2 $state2
    Assert ($script:messages[-1] -like '*up to date*') 'GUI never offers a downgrade'
    Invoke-GuiRollback $client2 $state2
    Assert ($script:messages[-2] -like 'Replace Astral Client 0.1.1 with*' -and ([IO.File]::ReadAllText($exe2)) -eq 'old client') 'GUI rollback'
    Assert (@(Get-ChildItem -LiteralPath (Join-Path $state2 'backups') -Directory).Count -eq 0) 'rolled-back backup pruned'
    Invoke-Install $valid $client2 $state2 | Out-Null
    Remove-StaleUpdates $state2
    Assert (@(Get-ChildItem -LiteralPath (Join-Path $state2 'backups') -Directory).Count -eq 1) 'rollback backup retained'
    Write-Host "$script:passed assertions passed"
} finally { Remove-Item -LiteralPath $root -Recurse -Force }
