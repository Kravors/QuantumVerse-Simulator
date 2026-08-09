$ErrorActionPreference = 'Stop'
$token = [System.Environment]::GetEnvironmentVariable('GH_TOKEN','Process')
if (-not $token) { throw 'GH_TOKEN not set in process environment. Run gh-login.ps1 first.' }
$headers = @{ Authorization = "Bearer $token"; Accept = 'application/vnd.github+json' }

$outDir = 'F:\syyyy\failure-analysis'
if (-not (Test-Path $outDir)) { New-Item -ItemType Directory -Path $outDir | Out-Null }

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
Write-Host "Processing $($failures.Count) failed runs..."

$results = @()
$processed = 0
foreach ($run in $failures) {
    $processed++
    if ($processed % 20 -eq 0) { Write-Host "Progress: $processed / $($failures.Count)" }
    try {
        $jobsUrl = "https://api.github.com/repos/Kravors/QuantumVerse-Simulator/actions/runs/$($run.Id)/jobs?per_page=100"
        $jobsResp = Invoke-RestMethod -Uri $jobsUrl -Headers $headers -Method Get
        $failedJobs = $jobsResp.jobs | Where-Object { $_.conclusion -eq 'failure' }
        foreach ($job in $failedJobs) {
            $logPath = Join-Path $outDir "$($run.Id)_$($job.Id)_$($job.name -replace '[^a-zA-Z0-9_-]','_').log"
            if (-not (Test-Path $logPath)) {
                $logUrl = "https://api.github.com/repos/Kravors/QuantumVerse-Simulator/actions/jobs/$($job.Id)/logs"
                Invoke-WebRequest -Uri $logUrl -Headers $headers -OutFile $logPath -UseBasicParsing | Out-Null
            }
            $log = Get-Content $logPath -Raw -ErrorAction SilentlyContinue
            $results += [pscustomobject]@{
                RunId = $run.Id
                RunName = $run.Name
                RunUrl = $run.Url
                JobId = $job.Id
                JobName = $job.name
                JobUrl = $job.html_url
                LogPath = $logPath
                ErrorLines = ($log -split "`n" | Select-String -Pattern '(?i)(error|exception|fatal|failed|traceback|assert|segmentation fault|coredump|linker error|compile error)' | Select-Object -First 20).Line
                Steps = ($job.steps | ForEach-Object { "$($_.name)::$($_.status)::$($_.conclusion)" }) -join ' | '
            }
        }
    } catch {
        Write-Warning "Failed to process run $($run.Id): $_"
    }
}
$results | Export-Csv -Path 'F:\syyyy\failure-analysis\failure-summary.csv' -NoTypeInformation -Encoding utf8
Write-Host "Done. Processed $($results.Count) failed jobs. Output: F:\syyyy\failure-analysis\failure-summary.csv"
