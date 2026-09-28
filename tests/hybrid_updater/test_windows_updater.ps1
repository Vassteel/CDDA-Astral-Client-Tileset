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
    Write-Host "$script:passed assertions passed"
} finally { Remove-Item -LiteralPath $root -Recurse -Force }
