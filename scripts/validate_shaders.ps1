# validate_shaders.ps1
# PowerShell wrapper for glslangValidator to bypass CMake quoting issues.
param(
    [Parameter(Mandatory=$true)]
    [string]$ValidatorPath,
    [Parameter(Mandatory=$true)]
    [string]$ShaderRoot,
    [Parameter(ValueFromRemainingArguments=$true)]
    [string[]]$ShaderFiles
)

$ErrorActionPreference = "Stop"
$exitCode = 0

foreach ($file in $ShaderFiles) {
    Write-Host "Validating $file..."
    & $ValidatorPath --auto-map-locations --client opengl100 -I"$ShaderRoot/common" $file
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Validation failed for $file"
        $exitCode = 1
    }
}
exit $exitCode
