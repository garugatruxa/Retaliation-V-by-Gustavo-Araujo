# Compila o Retaliation.asi (64 bits) com llvm-mingw.
# Uso: .\build.ps1            -> compila em build\
#      .\build.ps1 -Install   -> compila e copia para a pasta do GTA V Enhanced
param([switch]$Install,
      [string]$GameDir = "C:\Program Files (x86)\Steam\steamapps\common\Grand Theft Auto V Enhanced")

$ErrorActionPreference = "Continue"
$root = $PSScriptRoot
New-Item -ItemType Directory -Force "$root\build" | Out-Null
$src = Get-ChildItem "$root\src\*.cpp" | ForEach-Object { $_.FullName }
$flags = @("-std=c++20", "-O2", "-shared", "-static", "-s", "-Wall", "-Wno-writable-strings",
           "-Wno-unused-function", "-Wno-unused-variable", "-o", "$root\build\Retaliation.asi")
& clang++ @src @flags
if ($LASTEXITCODE -ne 0) { throw "Falha na compilacao" }
Write-Host "OK: build\Retaliation.asi"

if ($Install) {
    if (-not (Test-Path "$GameDir")) { throw "Pasta do jogo nao encontrada: $GameDir" }
    Copy-Item "$root\build\Retaliation.asi" "$GameDir\" -Force
    Write-Host "Instalado em $GameDir"
}

