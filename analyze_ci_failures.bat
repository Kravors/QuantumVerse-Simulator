@echo off
setlocal enabledelayedexpansion

echo ============================================
echo QuantumVerse CI Failure Analyzer
echo ============================================
echo.

REM Check if gh is installed
where gh >nul 2>nul
if %errorlevel% neq 0 (
    echo ERROR: GitHub CLI (gh) is not installed or not in PATH.
    echo Install it from: https://cli.github.com/
    pause
    exit /b 1
)

REM Check authentication status
echo [1/5] Checking GitHub CLI authentication...
gh auth status >nul 2>nul
if %errorlevel% neq 0 (
    echo GitHub CLI is not authenticated.
    echo Please run: gh auth login
    echo.
    echo You will need a GitHub Personal Access Token with 'repo' and 'actions:read' scopes.
    echo.
    pause
    exit /b 1
)
echo Authentication OK.
echo.

REM Configuration
set REPO=Kravors/QuantumVerse-Simulator
set OUTPUT_DIR=ci_analysis_output
set MAX_RUNS=200
set SUMMARY_FILE=%OUTPUT_DIR%\failure_summary.txt

REM Create output directory
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"
if not exist "%OUTPUT_DIR%\logs" mkdir "%OUTPUT_DIR%\logs"
if not exist "%OUTPUT_DIR%\runs" mkdir "%OUTPUT_DIR%\runs"

echo [2/5] Fetching recent workflow runs (limit: %MAX_RUNS%)...
gh run list --repo %REPO% --limit %MAX_RUNS% --json databaseId,conclusion,createdAt,headBranch,name,status,workflowName > "%OUTPUT_DIR%\all_runs.json"
if %errorlevel% neq 0 (
    echo ERROR: Failed to fetch workflow runs.
    pause
    exit /b 1
)

echo [3/5] Filtering failed runs...
python -c "
import json, sys
with open(r'%OUTPUT_DIR%\all_runs.json') as f:
    runs = json.load(f)
failed = [r for r in runs if r.get('conclusion') == 'failure']
print(f'Total runs: {len(runs)}, Failed: {len(failed)}')
with open(r'%OUTPUT_DIR%\failed_runs.json', 'w') as f:
    json.dump(failed, f, indent=2)
" 2>nul
if %errorlevel% neq 0 (
    echo ERROR: Python not found or failed to parse JSON. Install Python 3.
    pause
    exit /b 1
)

echo [4/5] Downloading job logs for failed runs...
python -c "
import json, subprocess, os, sys

with open(r'%OUTPUT_DIR%\failed_runs.json') as f:
    failed = json.load(f)

os.makedirs(r'%OUTPUT_DIR%\logs', exist_ok=True)
os.makedirs(r'%OUTPUT_DIR%\runs', exist_ok=True)

for run in failed:
    run_id = str(run['databaseId'])
    run_name = run.get('name', 'unknown')
    print(f'Processing run {run_id}: {run_name}')
    
    # List jobs for this run
    try:
        result = subprocess.run(
            ['gh', 'run', 'view', run_id, '--repo', r'%REPO%', '--json', 'jobs', '--jq', '.jobs'],
            capture_output=True, text=True, timeout=60
        )
        if result.returncode != 0:
            print(f'  Failed to get jobs for run {run_id}: {result.stderr[:200]}')
            continue
        
        jobs = json.loads(result.stdout)
        failed_jobs = [j for j in jobs if j.get('conclusion') == 'failure']
        
        if not failed_jobs:
            print(f'  No failed jobs found in run {run_id}')
            continue
        
        # Save run metadata
        with open(r'%OUTPUT_DIR%\runs\run_%s.json' % run_id, 'w') as f:
            json.dump({'run': run, 'jobs': jobs, 'failed_jobs': failed_jobs}, f, indent=2)
        
        # Download logs for failed jobs
        for job in failed_jobs:
            job_id = str(job['id'])
            job_name = job.get('name', 'unknown').replace('/', '_').replace(' ', '_')
            log_file = r'%OUTPUT_DIR%\logs\run_%s_job_%s.log' % (run_id, job_id)
            
            log_result = subprocess.run(
                ['gh', 'run', 'view', run_id, '--repo', r'%REPO%', '--log', '--job', job_id],
                capture_output=True, text=True, timeout=120
            )
            if log_result.returncode == 0:
                with open(log_file, 'w', encoding='utf-8') as lf:
                    lf.write(log_result.stdout)
                print(f'  Saved log: {log_file}')
            else:
                print(f'  Failed to download log for job {job_id}: {log_result.stderr[:200]}')
                
    except Exception as e:
        print(f'  Error processing run {run_id}: {e}')

print('Done downloading logs.')
" 

if %errorlevel% neq 0 (
    echo ERROR: Failed to download logs.
    pause
    exit /b 1
)

echo [5/5] Generating summary report...
python -c "
import json, os, re
from collections import Counter

output_dir = r'%OUTPUT_DIR%'
with open(os.path.join(output_dir, 'failed_runs.json')) as f:
    failed = json.load(f)

with open(os.path.join(output_dir, 'all_runs.json')) as f:
    all_runs = json.load(f)

# Count by workflow
workflow_counts = Counter(r.get('name', 'unknown') for r in failed)
branch_counts = Counter(r.get('headBranch', 'unknown') for r in failed)

summary_lines = []
summary_lines.append('=' * 60)
summary_lines.append('CI FAILURE ANALYSIS SUMMARY')
summary_lines.append('=' * 60)
summary_lines.append('')
summary_lines.append(f'Total runs analyzed: {len(all_runs)}')
summary_lines.append(f'Failed runs: {len(failed)}')
summary_lines.append(f'Failure rate: {len(failed)/len(all_runs)*100:.1f}%%')
summary_lines.append('')
summary_lines.append('TOP FAILING WORKFLOWS:')
for wf, count in workflow_counts.most_common(10):
    summary_lines.append(f'  {wf}: {count} failures')
summary_lines.append('')
summary_lines.append('FAILED BRANCHES:')
for branch, count in branch_counts.most_common(10):
    summary_lines.append(f'  {branch}: {count} failures')
summary_lines.append('')
summary_lines.append('RECENT FAILED RUNS:')
for r in failed[:10]:
    summary_lines.append(f'  Run #{r[\"databaseId\"]}: {r.get(\"name\", \"unknown\")} ({r.get(\"headBranch\", \"?\")}) - {r.get(\"createdAt\", \"?\")}')
summary_lines.append('')
summary_lines.append('=' * 60)
summary_lines.append('For detailed logs, check: ci_analysis_output/logs/')
summary_lines.append('For run metadata, check: ci_analysis_output/runs/')
summary_lines.append('=' * 60)

with open(os.path.join(output_dir, 'failure_summary.txt'), 'w') as f:
    f.write('\n'.join(summary_lines))

print('\n'.join(summary_lines))
"

if %errorlevel% neq 0 (
    echo ERROR: Failed to generate summary.
    pause
    exit /b 1
)

echo.
echo ============================================
echo ANALYSIS COMPLETE
echo ============================================
echo Summary: %SUMMARY_FILE%
echo Logs: %OUTPUT_DIR%\logs\
echo Run metadata: %OUTPUT_DIR%\runs\
echo.
type "%SUMMARY_FILE%"
echo.
pause
