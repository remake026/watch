param([string]$ReferenceAppPath)

$ErrorActionPreference = 'Stop'
$repoDir = Split-Path -Parent $PSScriptRoot
$bspDir = Join-Path $repoDir 'bsp'
$stubDir = Join-Path $PSScriptRoot 'host_stubs'
$testDir = Join-Path ([System.IO.Path]::GetTempPath()) ('watch-page-tests-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $testDir | Out-Null
$testFile = Join-Path $PSScriptRoot 'page_flow_test.c'
$gameFile = Join-Path $bspDir 'game.c'
$dataFile = Join-Path $bspDir 'OLED_Data.c'
$pageFiles = @('app', 'home_page', 'settings_page', 'menu_page', 'stopwatch_page',
               'game_page', 'animation_page', 'oled_test_page') | ForEach-Object { Join-Path $bspDir ($_.ToString() + '.c') }
$exeFile = Join-Path $testDir 'pages.exe'
$flags = @('-std=c99', '-Wall', '-Wextra', '-Wno-type-limits', '-Wno-unused-variable',
           '-Wno-unused-parameter', '-Wno-missing-braces', '-O2', '-I', $stubDir, '-I', $bspDir)
& gcc @flags $testFile @pageFiles $gameFile $dataFile -o $exeFile
if ($LASTEXITCODE -ne 0) { throw 'Page test compilation failed.' }

if ($ReferenceAppPath) {
    $referenceExe = Join-Path $testDir 'reference.exe'
    & gcc @flags $testFile $ReferenceAppPath $gameFile $dataFile -o $referenceExe
    if ($LASTEXITCODE -ne 0) { throw 'Reference compilation failed.' }
}

foreach ($scenario in 0..8) {
    $actual = & $exeFile $scenario
    if ($LASTEXITCODE -ne 0) { throw "Page scenario $scenario failed." }
    if ($ReferenceAppPath) {
        $expected = & $referenceExe $scenario
        if ($LASTEXITCODE -ne 0) { throw "Reference scenario $scenario failed." }
        if (($actual -join "`n") -cne ($expected -join "`n")) {
            throw "Page scenario $scenario differs from the original app: $actual / $expected"
        }
    }
    Write-Output $actual
}
if ($ReferenceAppPath) { Write-Output 'All page traces match the original application.' }
