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
    # v10: sem recarregar com o jogo aberto (o Ctrl+R do ScriptHookV.dev nao funciona no Enhanced).
    # Feche o jogo antes de instalar.
    if (Get-Process GTA5_Enhanced -ErrorAction SilentlyContinue) { throw "Feche o GTA V antes de instalar." }
    $dst = "$GameDir\Retaliation.asi"
    Get-ChildItem "$GameDir\Retaliation.asi.old*" -ErrorAction SilentlyContinue | Remove-Item -ErrorAction SilentlyContinue
    Copy-Item "$root\build\Retaliation.asi" $dst -Force
    Write-Host "Instalado em $GameDir"
}

