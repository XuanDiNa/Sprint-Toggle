param([string]$Stamp,[string]$DllPath)
$ErrorActionPreference='Stop'
$project=Split-Path $PSScriptRoot -Parent
if(!$Stamp){$Stamp=Get-Date -Format 'yyyyMMdd-HHmmss'}
if($Stamp -notmatch '^\d{8}-\d{6}$'){throw 'Invalid package timestamp'}
$version='0.1.0-test7-leftalt'
$dll=if($DllPath){(Resolve-Path -LiteralPath $DllPath).ProviderPath}else{Join-Path $project 'build\windows\x64\release\ToggleSprint.dll'}
# Preserve the exact consumer already frozen in the matched seven-plugin bundle.
if((Get-FileHash -LiteralPath $dll -Algorithm SHA256).Hash -ne '09C9BD5051F6E67E889BEEC55ECF976940FBEE45831D037488726C32CED0F2EB'){throw 'DLL differs from frozen matched test7; do not silently substitute a rebuild'}
$release=Join-Path $project ('releases\ToggleSprint-'+$version+'-bindings-followup-'+$Stamp)
if(Test-Path -LiteralPath $release){throw 'Release directory already exists; preserving it'}
$payload=Join-Path $release 'ToggleSprint'
New-Item -ItemType Directory -Path $payload | Out-Null
$files=@('manifest.json','mod.json','config.json','config.schema.json','modmenu.settings.json','README.md','TESTING.md','THIRD_PARTY_NOTICES.txt')
foreach($name in $files){Copy-Item -LiteralPath (Join-Path $project $name) -Destination (Join-Path $payload $name)}
Copy-Item -LiteralPath $dll -Destination (Join-Path $payload 'ToggleSprint.dll')
$sums=foreach($file in Get-ChildItem -LiteralPath $payload -File | Sort-Object Name){(Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash+'  '+$file.Name}
[IO.File]::WriteAllLines((Join-Path $payload 'SHA256SUMS.txt'),$sums,[Text.Encoding]::ASCII)
$zip=Join-Path $release ('ToggleSprint-'+$version+'-mc1.26.52.03-win-x64-full.zip')
Compress-Archive -LiteralPath $payload -DestinationPath $zip
& py (Join-Path $PSScriptRoot 'verify_artifacts.py') --dll $dll --zip $zip
if($LASTEXITCODE -ne 0){throw 'Frozen ZIP verification failed'}
$updatePayload=Join-Path $release 'update\ToggleSprint'
New-Item -ItemType Directory -Path $updatePayload | Out-Null
foreach($file in Get-ChildItem -LiteralPath $payload -File | Where-Object {$_.Name -notin @('config.json','SHA256SUMS.txt')}){
    Copy-Item -LiteralPath $file.FullName -Destination (Join-Path $updatePayload $file.Name)
}
$updateSums=foreach($file in Get-ChildItem -LiteralPath $updatePayload -File | Sort-Object Name){(Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash+'  '+$file.Name}
[IO.File]::WriteAllLines((Join-Path $updatePayload 'SHA256SUMS.txt'),$updateSums,[Text.Encoding]::ASCII)
$updateZip=Join-Path $release ('ToggleSprint-'+$version+'-mc1.26.52.03-win-x64-update.zip')
Compress-Archive -LiteralPath $updatePayload -DestinationPath $updateZip
& py (Join-Path $PSScriptRoot 'verify_artifacts.py') --dll $dll --update-zip $updateZip
if($LASTEXITCODE -ne 0){throw 'Config-preserving update ZIP verification failed'}
foreach($archive in @($zip,$updateZip)){
    $line=(Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash+'  '+(Split-Path $archive -Leaf)
    [IO.File]::WriteAllText(($archive+'.sha256'),($line+"`n"),[Text.Encoding]::ASCII)
}
Get-FileHash -LiteralPath (Join-Path $payload 'ToggleSprint.dll'),$zip,$updateZip -Algorithm SHA256 | Format-List
Write-Output ('PACKAGE '+$zip)
Write-Output ('UPDATE PACKAGE '+$updateZip)
