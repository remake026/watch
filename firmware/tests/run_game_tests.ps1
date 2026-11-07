$ErrorActionPreference = 'Stop'
$repoDir = Split-Path -Parent $PSScriptRoot
$testDir = Join-Path ([System.IO.Path]::GetTempPath()) ('watch-game-tests-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $testDir | Out-Null
$bspDir = Join-Path $repoDir 'bsp'
$gameFile = Join-Path $bspDir 'game.c'
$dataFile = Join-Path $bspDir 'OLED_Data.c'
$logicExe = Join-Path $testDir 'game_logic.exe'
$renderExe = Join-Path $testDir 'game_render.exe'

function Read-Function([string]$source, [string]$name) {
    $match = [regex]::Match($source, '(?m)^(?:static )?(?:void|uint8_t) ' + $name + '\([^\r\n]*\)\r?\n\{')
    if (!$match.Success) { throw "Function not found: $name" }
    $start = $match.Index
    $at = $source.IndexOf('{', $start)
    $depth = 1
    for ($at++; $depth -gt 0; $at++) {
        if ($source[$at] -eq '{') { $depth++ }
        if ($source[$at] -eq '}') { $depth-- }
    }
    return $source.Substring($start, $at - $start)
}

& gcc -std=c99 -Wall -Wextra -Werror -Wno-missing-braces -O2 -I $bspDir (Join-Path $PSScriptRoot 'game_test.c') $gameFile $dataFile -o $logicExe
if ($LASTEXITCODE -ne 0) { throw 'Game logic test compilation failed.' }
& $logicExe
if ($LASTEXITCODE -ne 0) { throw 'Game logic tests failed.' }

$oledSource = Get-Content -LiteralPath (Join-Path $bspDir 'OLED.c') -Encoding UTF8 -Raw
$pageSource = Get-Content -LiteralPath (Join-Path $bspDir 'game_page.c') -Encoding UTF8 -Raw
$functions = @()
foreach ($name in @('OLED_Clear', 'OLED_ClearArea', 'OLED_ShowImage', 'OLED_ShowPandaRun',
                    'OLED_DrawPoint', 'OLED_DrawLine', 'OLED_ShowChar', 'OLED_ShowString', 'OLED_Printf')) {
    $functions += Read-Function $oledSource $name
}
foreach ($name in @('GameUpdateArea', 'GameDrawSprites', 'GameDrawScore', 'GameDrawScene', 'GameDrawChanges', 'GamePage_Run')) {
    $body = Read-Function $pageSource $name
    if ($name -eq 'GameDrawSprites') {
        $at = $body.IndexOf('{') + 1
        $body = $body.Insert($at, "`n    rendered = *game;")
    }
    if ($name -eq 'GameDrawScore') {
        $at = $body.IndexOf('{') + 1
        $body = $body.Insert($at, "`n    renderedBest = best;")
    }
    $functions += $body
}
[System.IO.File]::WriteAllText((Join-Path $testDir 'game_render_impl.inc'), ($functions -join "`n"))
& gcc -std=c99 -Wall -Wextra -Wno-missing-braces -O2 -I $bspDir -I $testDir (Join-Path $PSScriptRoot 'game_render_test.c') $gameFile $dataFile -o $renderExe
if ($LASTEXITCODE -ne 0) { throw 'Game rendering test compilation failed.' }
& $renderExe
if ($LASTEXITCODE -ne 0) { throw 'Game rendering tests failed.' }
