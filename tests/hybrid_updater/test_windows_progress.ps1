param([string]$Updater, [string]$Output)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
. $Updater
[IO.Directory]::CreateDirectory($Output) | Out-Null
$script:cancelProgress = $false
$script:captures = 0
# Exercise the real WinForms UI with deterministic download progress and no network writes.
function Save-Download($Release, [string]$State, [string]$Repo, [scriptblock]$OnProgress) {
    & $OnProgress 5MB 10MB
    $form = @([Windows.Forms.Application]::OpenForms | Where-Object { $_.Text -eq 'Astral Client' })[0]
    if (!$form -or !$form.Visible) { throw 'Progress window did not appear' }
    $bar = @($form.Controls | Where-Object { $_ -is [Windows.Forms.ProgressBar] })[0]
    if ($bar.Value -ne 50) { throw 'Progress bar did not reach 50 percent' }
    $image = New-Object Drawing.Bitmap($form.Width, $form.Height)
    try {
        $form.DrawToBitmap($image, (New-Object Drawing.Rectangle(0, 0, $form.Width, $form.Height)))
        $image.Save((Join-Path $Output "progress-$script:captures.png"), [Drawing.Imaging.ImageFormat]::Png)
        $script:captures++
    } finally { $image.Dispose() }
    if ($script:cancelProgress) {
        $button = @($form.Controls | Where-Object { $_ -is [Windows.Forms.Button] -and $_.Text -eq 'Cancel' })[0]
        $button.PerformClick()
        & $OnProgress 6MB 10MB
        throw 'Cancel did not stop progress'
    }
    & $OnProgress 10MB 10MB
    return 'fixture-download.zip'
}
$release = @{version='client-v0.1.3'}
$result = Save-DownloadWithProgress $release $Output 'fixture/repo'
if ($result -ne 'fixture-download.zip') { throw 'Completed progress lost its result' }
if ([Windows.Forms.Application]::OpenForms.Count -ne 0) { throw 'Completed window leaked' }
$script:cancelProgress = $true
$result = Save-DownloadWithProgress $release $Output 'fixture/repo'
if ($null -ne $result) { throw 'Cancelled download returned a package' }
if ([Windows.Forms.Application]::OpenForms.Count -ne 0) { throw 'Cancelled window leaked' }
@{completion='passed'; cancellation='passed'; window_disposal='passed'; powershell=$PSVersionTable.PSVersion.ToString()} | ConvertTo-Json | Set-Content -Encoding UTF8 (Join-Path $Output 'result.json')
Write-Host 'Native Windows progress window: completion and cancellation passed'
