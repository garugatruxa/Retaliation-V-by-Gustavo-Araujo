# Compila o Retaliation.asi (64 bits) com llvm-mingw.
# Uso: .\build.ps1            -> compila em build\
#      .\build.ps1 -Install   -> compila e copia para a pasta do GTA V Enhanced
param([switch]$Install,
      [string]$GameDir = "C:\Program Files (x86)\Steam\steamapps\common\Grand Theft Auto V Enhanced")

$ErrorActionPreference = "Continue"
$root = $PSScriptRoot
New-Item -ItemType Directory -Force "$root\build" | Out-Null
$src = Get-ChildItem "$root\src\*.cpp" | ForEach-Object { $_.FullName }
$flags = @("-std=c++20", "-DRET_SPAWN", "-O2", "-shared", "-static", "-s", "-Wall", "-Wno-writable-strings",
           "-Wno-unused-function", "-Wno-unused-variable", "-o", "$root\build\Retaliation.asi")
& clang++ @src @flags
if ($LASTEXITCODE -ne 0) { throw "Falha na compilacao" }
Write-Host "OK: build\Retaliation.asi"

if ($Install) {
    if (-not (Test-Path "$GameDir")) { throw "Pasta do jogo nao encontrada: $GameDir" }
    # com o jogo aberto o .asi fica travado, mas pode ser renomeado; depois e so apertar Ctrl+R no jogo
    $dst = "$GameDir\Retaliation.asi"
    Get-ChildItem "$GameDir\Retaliation.asi.old*" -ErrorAction SilentlyContinue | Remove-Item -ErrorAction SilentlyContinue
    if (Test-Path $dst) {
        try { Remove-Item $dst -ErrorAction Stop }
        catch { Rename-Item $dst ("Retaliation.asi.old" + (Get-Date -Format "HHmmss")) }
    }
    Copy-Item "$root\build\Retaliation.asi" $dst -Force
    Write-Host "Instalado em $GameDir"
}

