param(
    [string]$OriginalImage,
    [string]$LauncherImage,
    [string]$ModMenuSource,
    [string]$FrozenDll,
    [switch]$Package
)
$ErrorActionPreference='Stop'
$project=Split-Path $PSScriptRoot -Parent
Set-Location -LiteralPath $project
$env:NO_COLOR='1'
$env:XMAKE_COLORTERM='nocolor'
function Run([string]$Program,[string[]]$Arguments){
    Write-Output ('RUN '+$Program+' '+($Arguments -join ' '))
    $output=@(& $Program @Arguments 2>&1)
    $exitCode=$LASTEXITCODE
    $output | ForEach-Object {Write-Output $_}
    if($exitCode -ne 0){throw ($Program+' failed')}
    if(($output -join "`n") -match '(?i)warning\s+(C|LNK)\d+|警告\s+(C|LNK)\d+'){throw 'Compiler/linker warning refused'}
}
if([bool]$OriginalImage -ne [bool]$LauncherImage){throw 'Supply both exact game images, or neither'}
if($Package -and !$FrozenDll){throw 'Packaging this release requires its frozen DLL'}
if($FrozenDll){$FrozenDll=(Resolve-Path -LiteralPath $FrozenDll).ProviderPath}
if($ModMenuSource){$ModMenuSource=(Resolve-Path -LiteralPath $ModMenuSource).ProviderPath}
$images=@()
if($OriginalImage){
    $OriginalImage=(Resolve-Path -LiteralPath $OriginalImage).ProviderPath
    $LauncherImage=(Resolve-Path -LiteralPath $LauncherImage).ProviderPath
    $images=@(
        @($OriginalImage,'4A92BFA3CE2428B40EE517B7C1125D9F6F79274382DE03846351992663B2E2E4'),
        @($LauncherImage,'0B66101DC873434E853A898329CB85CEE0DE52D0AE91AC796EFA0FE05B61522C')
    )
    foreach($item in $images){if((Get-FileHash -LiteralPath $item[0] -Algorithm SHA256).Hash -ne $item[1]){throw 'Game image identity refused'}}
}
$suites=@('Config','KeyToken','Policy','Profile','NativeInput','ConfigFile','Smoke')
if($ModMenuSource){$suites+='ModMenu'}
foreach($mode in @('debug','release')){
    $configure=@('f','-c','-p','windows','-a','x64','-m',$mode,'-y')
    if($ModMenuSource){$configure+=('--modmenu_source='+$ModMenuSource)}
    Run 'xmake' $configure
    Run 'xmake' @('build','-r','-y')
    foreach($suite in $suites){
        $arguments=@('run',('ToggleSprint'+$suite+'Tests'))
        if($suite -eq 'Profile' -and $OriginalImage){$arguments+=@($OriginalImage,$LauncherImage)}
        Run 'xmake' $arguments
    }
    if($FrozenDll){Run 'xmake' @('run','ToggleSprintSmokeTests',$FrozenDll)}
}
Run 'py' @('tools/test_config_schema.py')
Run 'py' @('tools/verify_metadata.py')
Run 'py' @('tools/verify_artifacts.py')
Run 'py' @('tools/test_artifact_verifier.py')
foreach($item in $images){if((Get-FileHash -LiteralPath $item[0] -Algorithm SHA256).Hash -ne $item[1]){throw 'Game image changed'}}
if($FrozenDll){Run 'py' @('tools/verify_artifacts.py','--dll',$FrozenDll)}
Run 'git' @('diff','--check')
if($Package){& (Join-Path $PSScriptRoot 'package_release.ps1') -DllPath $FrozenDll}
Write-Output 'VERIFICATION PASSED: public source offline checks; gameplay acceptance remains pending'
