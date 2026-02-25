$vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"

if (-not (Test-Path $vsWhere)) {
    Write-Host "Error: vswhere.exe not found" -ForegroundColor Red
    exit 1
}

$vsDir = & $vsWhere -latest -property installationPath

if (-not $vsDir) {
    Write-Host "Error: Visual Studio not found" -ForegroundColor Red
    exit 1
}

$vcvarsall = Join-Path $vsDir "VC\Auxiliary\Build\vcvarsall.bat"

if (-not (Test-Path $vcvarsall)) {
    Write-Host "Error: vcvarsall.bat not found at $vcvarsall" -ForegroundColor Red
    exit 1
}

$config = $args[0]
$buildDir = "out/windows-client-$config/build"

Write-Host "Configuring Windows MSVC Client ($config)..."
cmd /c "`"$vcvarsall`" x64 >NUL 2>&1 && cmake -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl -S . -B $buildDir -G `"Ninja Multi-Config`""

if ($LASTEXITCODE -ne 0) {
    Write-Host "Configure failed!" -ForegroundColor Red
    exit $LASTEXITCODE
}

Write-Host "Building Windows MSVC Client ($config)..."
cmd /c "`"$vcvarsall`" x64 >NUL 2>&1 && cmake --build $buildDir --config $config"

if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed!" -ForegroundColor Red
    exit $LASTEXITCODE
}

Write-Host "Build complete!" -ForegroundColor Green
