$tests = @(
    'test_vector4d',
    'test_matrix4x4',
    'test_property_based',
    'test_ui4d',
    'test_geodesic',
    'test_spacetime',
    'test_gr_validation',
    'test_mercury_precession',
    'test_light_deflection',
    'test_gravitational_redshift',
    'test_frame_dragging',
    'test_cdt',
    'test_spin_foam',
    'test_singularity_handler',
    'test_scenario_integration',
    'test_autodiff_validation',
    'test_exact_solutions',
    'test_quantum_gravity_consistency',
    'test_black_hole_jet_anomaly',
    'test_boson_star_collision',
    'test_exoplanetary_ttv',
    'test_fine_structure_drift',
    'test_galactic_rotation_curve',
    'test_galactic_tidal_stream',
    'test_neutron_glitch',
    'test_primordial_lithium',
    'test_ultralight_dm_wave',
    'test_discovery_panel_manager',
    'test_discovery_robustness',
    'test_rl_agent',
    'test_gcn_parser',
    'test_gcn_replay',
    'test_kafka_listener',
    'test_multimessenger_correlator',
    'test_frame_diagnostics',
    'test_performance_regression'
)

$results = @()

foreach ($t in $tests) {
    $exe = "build\Debug\$t.exe"
    if (-not (Test-Path $exe)) {
        $results += "$t|MISSING"
        continue
    }
    
    $job = Start-Job -ScriptBlock {
        param($exe)
        $p = Start-Process -FilePath $exe -NoNewWindow -Wait -PassThru -RedirectStandardOutput "$exe.out" -RedirectStandardError "$exe.err"
        return $p.ExitCode
    } -ArgumentList $exe
    
    $completed = Wait-Job $job -Timeout 30
    if ($completed) {
        $exitCode = Receive-Job $job
        $results += "$t|$exitCode"
    } else {
        Stop-Job $job
        Remove-Job $job
        $results += "$t|TIMEOUT"
    }
    Remove-Job $job -ErrorAction SilentlyContinue
}

$results | ForEach-Object { Write-Output $_ }
