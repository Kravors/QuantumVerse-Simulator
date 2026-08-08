$ErrorActionPreference = 'Stop'
$token = [System.Environment]::GetEnvironmentVariable('GH_TOKEN','Process')
if (-not $token) { throw 'GH_TOKEN not set in process environment. Run gh-login.ps1 first.' }
$headers = @{ Authorization = "Bearer $token"; Accept = 'application/vnd.github+json' }

$runs = Get-Content 'F:\syyyy\runs.tsv' | Select-Object -First 300 | ForEach-Object {
    $parts = $_ -split "`t"
    [pscustomobject]@{
        Id = [long]$parts[0]
        Name = $parts[1]
        Conclusion = $parts[2]
        CreatedAt = $parts[3]
        Url = $parts[4]
    }
}

$failures = $runs | Where-Object { $_.Conclusion -eq 'failure' }
Write-Host "Latest 300 runs: $($runs.Count) total, $($failures.Count) failed"

$summary = $failures | Group-Object Name | Sort-Object Count -Descending | Select-Object Count, Name
$summary | Format-Table -AutoSize
