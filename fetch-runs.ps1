$ErrorActionPreference = 'Stop'
$token = [System.Environment]::GetEnvironmentVariable('GH_TOKEN','Process')
if (-not $token) {
    $secure = Read-Host -Prompt 'Paste GitHub token' -AsSecureString
    $token = [Runtime.InteropServices.Marshal]::PtrToStringAuto([Runtime.InteropServices.Marshal]::SecureStringToBSTR($secure))
    [System.Environment]::SetEnvironmentVariable('GH_TOKEN', $token, 'Process')
}
$headers = @{ Authorization = "Bearer $token"; Accept = 'application/vnd.github+json' }
$runs = @()
$page = 1
do {
    $url = "https://api.github.com/repos/Kravors/QuantumVerse-Simulator/actions/runs?per_page=100&page=$page"
    $resp = Invoke-RestMethod -Uri $url -Headers $headers -Method Get
    $items = $resp.workflow_runs
    if (-not $items) { break }
    $runs += $items
    $page++
    if ($items.Count -lt 100) { break }
} while ($page -le 10)
$rows = $runs | ForEach-Object { "$($_.id)`t$($_.name)`t$($_.conclusion)`t$($_.created_at)`t$($_.html_url)" }
$rows | Out-File -Encoding utf8 'F:\syyyy\runs.tsv'
Write-Host "Fetched $($runs.Count) runs. Saved to F:\syyyy\runs.tsv"
