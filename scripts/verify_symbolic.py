#!/usr/bin/env python3
"""
Symbolic computer algebra cross-check for QuantumVerse.

Compares SymPy-derived / analytic curvature invariants against the C++
implementation for Schwarzschild, Kerr, FLRW (flat matter-dominated), and
CGHS (2D dilaton) metrics on random grids.

Usage:
    python scripts/verify_symbolic.py [--points N] [--seed S] [--binary PATH]

Exit code:
    0 on success, 1 on any assertion failure.
"""

import argparse
import json
import math
import os
import random
import subprocess
import sys
import time
import pathlib

try:
    import sympy as sp
except ImportError as e:
    print(json.dumps({"warning": f"SymPy not available: {e}; skipping symbolic verification"}, indent=2))
    sys.exit(0)


# ---------------------------------------------------------------------------
# SymPy symbolic setup (geometric units: G = c = 1)
# ---------------------------------------------------------------------------

def build_symbolic_schwarzschild(M_val=1.0):
    t, r, theta, phi = sp.symbols('t r theta phi', real=True, positive=True)
    M = sp.symbols('M', positive=True)
    coords = [t, r, theta, phi]

    g = sp.diag(-(1 - 2*M/r), 1/(1 - 2*M/r), r**2, r**2*sp.sin(theta)**2)
    g_inv = g.inv()

    def christoffel(rho, mu, nu):
        expr = 0
        for sigma in range(4):
            expr += g_inv[rho, sigma] * (
                sp.diff(g[sigma, nu], coords[mu]) +
                sp.diff(g[sigma, mu], coords[nu]) -
                sp.diff(g[mu, nu], coords[sigma])
            )
        return sp.simplify(expr / 2)

    Gamma = sp.MutableDenseNDimArray.zeros(4, 4, 4)
    for rho in range(4):
        for mu in range(4):
            for nu in range(4):
                Gamma[rho, mu, nu] = christoffel(rho, mu, nu)

    def riemann(rho, sigma, mu, nu):
        expr = (
            sp.diff(Gamma[rho, nu, sigma], coords[mu]) -
            sp.diff(Gamma[rho, mu, sigma], coords[nu])
        )
        for lam in range(4):
            expr += Gamma[rho, mu, lam] * Gamma[lam, nu, sigma]
            expr -= Gamma[rho, nu, lam] * Gamma[lam, mu, sigma]
        return sp.simplify(expr)

    Riemann = sp.MutableDenseNDimArray.zeros(4, 4, 4, 4)
    for rho in range(4):
        for sigma in range(4):
            for mu in range(4):
                for nu in range(4):
                    Riemann[rho, sigma, mu, nu] = riemann(rho, sigma, mu, nu)

    def ricci(mu, nu):
        expr = 0
        for lam in range(4):
            expr += Riemann[lam, mu, lam, nu]
        return sp.simplify(expr)

    Ricci = sp.MutableDenseNDimArray.zeros(4, 4)
    for mu in range(4):
        for nu in range(4):
            Ricci[mu, nu] = ricci(mu, nu)

    RicciScalar = sp.simplify(sum(g_inv[mu, nu] * Ricci[mu, nu] for mu in range(4) for nu in range(4)))

    if RicciScalar != 0:
        print(json.dumps({
            "warning": "Symbolic Ricci scalar is not zero",
            "ricci_scalar": str(RicciScalar),
        }, indent=2))

    expected_kretsch = 48 * M**2 / r**6

    ricci_expr = RicciScalar.subs({M: M_val})
    kretsch_expr = expected_kretsch.subs({M: M_val})
    ricci_scalar_lambdified = sp.lambdify((r,), ricci_expr, 'numpy')
    kretschmann_lambdified = sp.lambdify((r,), kretsch_expr, 'numpy')

    return ricci_scalar_lambdified, kretschmann_lambdified


def build_symbolic_kerr(M_val=1.0, a_val=0.5):
    r, theta = sp.symbols('r theta', real=True, positive=True)
    M = sp.symbols('M', positive=True)
    a = sp.symbols('a', positive=True)

    cos2 = sp.cos(theta)**2
    sin2 = sp.sin(theta)**2
    r2 = r**2
    a2 = a**2
    sigma = r2 + a2 * cos2
    delta = r2 - 2*M*r + a2

    # Kerr metric in Boyer-Lindquist (only diagonal + g_tphi needed for curvature)
    # We use the known analytic Kretschmann scalar:
    # K = 48 M^2 (r^2 - a^2 cos^2(theta)) ((r^2 + a^2 cos^2(theta))^2 - 16 r^2 a^2 cos^2(theta)) / sigma^6
    num1 = r2 - a2 * cos2
    num2 = sigma**2 - 16 * r2 * a2 * cos2
    K_expr = 48 * M**2 * num1 * num2 / sigma**6

    # Ricci scalar = 0 for vacuum
    R_expr = 0

    K_sub = K_expr.subs({M: M_val, a: a_val})
    kretsch_lambdified = sp.lambdify((r, theta), K_sub, 'numpy')

    # Return a lambda that ignores theta for Ricci (always 0)
    ricci_lambdified = lambda r_val, theta_val: 0.0

    return ricci_lambdified, kretsch_lambdified


def build_symbolic_flrw_matter(t0_val=1.0):
    t = sp.symbols('t', positive=True)
    t0 = sp.symbols('t0', positive=True)

    # a(t) = (t/t0)^(2/3)
    # H = 2/(3t)
    # R = 4/(3t^2)
    # K = 16/(27 t^4)
    R_expr = sp.Rational(4, 3) / t**2
    K_expr = sp.Rational(16, 27) / t**4

    R_sub = R_expr.subs({t0: t0_val})
    K_sub = K_expr.subs({t0: t0_val})
    ricci_lambdified = sp.lambdify((t,), R_sub, 'numpy')
    kretsch_lambdified = sp.lambdify((t,), K_sub, 'numpy')

    return ricci_lambdified, kretsch_lambdified


def build_symbolic_cghs(M_val=1.0, sigma_val=0.0):
    x_plus, x_minus = sp.symbols('x_plus x_minus', real=True)
    M = sp.symbols('M', positive=True)
    sigma = sp.symbols('sigma', real=True)

    # A = 1 + M e^(2 sigma) x^+ x^-
    A = 1 + M * sp.exp(2 * sigma) * x_plus * x_minus
    # R = 4 M e^(2 sigma) / A^3
    R_expr = 4 * M * sp.exp(2 * sigma) / A**3
    # K = R^2 in 2D
    K_expr = R_expr**2

    R_sub = R_expr.subs({M: M_val, sigma: sigma_val})
    K_sub = K_expr.subs({M: M_val, sigma: sigma_val})
    ricci_lambdified = sp.lambdify((x_plus, x_minus), R_sub, 'numpy')
    kretsch_lambdified = sp.lambdify((x_plus, x_minus), K_sub, 'numpy')

    return ricci_lambdified, kretsch_lambdified


# ---------------------------------------------------------------------------
# C++ binary interaction
# ---------------------------------------------------------------------------

def find_binary(arg_path):
    if arg_path:
        p = pathlib.Path(arg_path)
        if p.exists():
            return str(p)
    candidates = [
        pathlib.Path(__file__).resolve().parent.parent / "build" / "Release" / "test_symbolic_verify.exe",
        pathlib.Path(__file__).resolve().parent.parent / "build" / "Release" / "test_symbolic_verify",
        pathlib.Path(__file__).resolve().parent.parent / "build" / "test_symbolic_verify",
        pathlib.Path("F:/syyyy/build/Release/test_symbolic_verify.exe"),
        pathlib.Path("F:/syyyy/build/test_symbolic_verify.exe"),
    ]
    for c in candidates:
        if c.exists():
            return str(c)
    return None


def call_cpp(binary, metric, params, point):
    payload = {
        "metric": metric,
        "params": params,
        "point": point,
    }
    try:
        proc = subprocess.run(
            [binary],
            input=json.dumps(payload) + "\n",
            capture_output=True,
            text=True,
            timeout=30,
        )
    except subprocess.TimeoutExpired:
        return None, "timeout"
    except FileNotFoundError:
        return None, "binary not found"

    if proc.returncode != 0:
        return None, f"exit {proc.returncode}: {proc.stderr.strip()}"

    stdout = proc.stdout.strip()
    if not stdout:
        return None, "empty output"

    try:
        return json.loads(stdout), None
    except json.JSONDecodeError as e:
        return None, f"json parse error: {e}"


# ---------------------------------------------------------------------------
# Main verification loop
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(description="SymPy vs C++ symbolic cross-check")
    parser.add_argument("--points", type=int, default=10000, help="Number of random points per metric")
    parser.add_argument("--seed", type=int, default=42, help="Random seed")
    parser.add_argument("--binary", type=str, default="", help="Path to C++ invariant exporter")
    parser.add_argument("--tol", type=float, default=1e-12, help="Absolute tolerance")
    args = parser.parse_args()

    binary = find_binary(args.binary)
    if not binary:
        print(json.dumps({"warning": "C++ binary not found; skipping symbolic verification", "hint": "Build test_symbolic_verify first"}, indent=2))
        sys.exit(0)

    metrics = [
        {
            "name": "schwarzschild",
            "params": {"M": 1.0},
            "sym_builder": lambda: build_symbolic_schwarzschild(1.0),
            "point_gen": lambda rng: _schwarzschild_point(rng),
        },
        {
            "name": "kerr",
            "params": {"M": 1.0, "a": 0.5},
            "sym_builder": lambda: build_symbolic_kerr(1.0, 0.5),
            "point_gen": lambda rng: _kerr_point(rng, 1.0, 0.5),
        },
        {
            "name": "flrw_matter",
            "params": {"t0": 1.0},
            "sym_builder": lambda: build_symbolic_flrw_matter(1.0),
            "point_gen": lambda rng: _flrw_point(rng, 1.0),
        },
        {
            "name": "cghs",
            "params": {"M": 1.0, "sigma": 0.0},
            "sym_builder": lambda: build_symbolic_cghs(1.0, 0.0),
            "point_gen": lambda rng: _cghs_point(rng, 1.0, 0.0),
        },
    ]

    total_passed = 0
    total_failed = 0
    overall_max_ricci = 0.0
    overall_max_kretsch = 0.0
    t0 = time.time()

    for metric in metrics:
        name = metric["name"]
        print(f"Compiling SymPy expressions for {name}...", file=sys.stderr)
        ricci_fn, kretsch_fn = metric["sym_builder"]()
        print(f"SymPy expressions ready for {name}.", file=sys.stderr)

        failures = 0
        max_abs_ricci = 0.0
        max_abs_kretsch = 0.0
        metric_t0 = time.time()

        for i in range(args.points):
            rng = random.Random(args.seed + i)
            point, cpp_params = metric["point_gen"](rng)

            sym_ricci = float(ricci_fn(*_sym_args(metric, point)))
            sym_kretsch = float(kretsch_fn(*_sym_args(metric, point)))

            cpp_out, err = call_cpp(binary, name, cpp_params, point)

            if err:
                print(json.dumps({
                    "metric": name,
                    "error": f"point {i}: {err}",
                    "point": point,
                }, indent=2))
                failures += 1
                continue

            cpp_ricci = cpp_out.get("ricci_scalar", float('nan'))
            cpp_kretsch = cpp_out.get("kretschmann", float('nan'))

            abs_ricci = abs(sym_ricci - cpp_ricci)
            abs_kretsch = abs(sym_kretsch - cpp_kretsch)
            max_abs_ricci = max(max_abs_ricci, abs_ricci)
            max_abs_kretsch = max(max_abs_kretsch, abs_kretsch)

            rel_tol = 1e-12
        abs_tol = args.tol
        effective_tol_ricci = max(abs_tol, rel_tol * max(abs(sym_ricci), abs(cpp_ricci), 1.0))
        effective_tol_kretsch = max(abs_tol, rel_tol * max(abs(sym_kretsch), abs(cpp_kretsch), 1.0))

        if abs_ricci > effective_tol_ricci or abs_kretsch > effective_tol_kretsch:
                failures += 1
                if failures <= 5:
                    print(json.dumps({
                        "metric": name,
                        "point_index": i,
                        "point": point,
                        "sym_ricci": sym_ricci,
                        "cpp_ricci": cpp_ricci,
                        "abs_ricci": abs_ricci,
                        "tol_ricci": effective_tol_ricci,
                        "sym_kretsch": sym_kretsch,
                        "cpp_kretsch": cpp_kretsch,
                        "abs_kretsch": abs_kretsch,
                        "tol_kretsch": effective_tol_kretsch,
                    }, indent=2))

        elapsed = time.time() - metric_t0
        passed = args.points - failures
        summary = {
            "metric": name,
            "points_checked": args.points,
            "passed": passed,
            "failed": failures,
            "max_abs_ricci": max_abs_ricci,
            "max_abs_kretsch": max_abs_kretsch,
            "elapsed_seconds": round(elapsed, 3),
            "tol": args.tol,
        }
        print(json.dumps(summary, indent=2))

        total_passed += passed
        total_failed += failures
        overall_max_ricci = max(overall_max_ricci, max_abs_ricci)
        overall_max_kretsch = max(overall_max_kretsch, max_abs_kretsch)

    elapsed = time.time() - t0
    total_summary = {
        "status": "complete",
        "total_points": args.points * len(metrics),
        "total_passed": total_passed,
        "total_failed": total_failed,
        "overall_max_abs_ricci": overall_max_ricci,
        "overall_max_abs_kretsch": overall_max_kretsch,
        "elapsed_seconds": round(elapsed, 3),
        "tol": args.tol,
    }
    print(json.dumps(total_summary, indent=2))

    if total_failed > 0:
        sys.exit(1)
    sys.exit(0)


# ---------------------------------------------------------------------------
# Point generators (return [point], cpp_params dict)
# ---------------------------------------------------------------------------

def _schwarzschild_point(rng, M=1.0):
    r = rng.uniform(2.1, 100.0)
    theta = rng.uniform(0.01, math.pi - 0.01)
    phi = rng.uniform(0.0, 2.0 * math.pi)
    t = rng.uniform(-1.0, 1.0)
    x = r * math.sin(theta) * math.cos(phi)
    y = r * math.sin(theta) * math.sin(phi)
    z = r * math.cos(theta)
    return [t, x, y, z], {"M": M}


def _kerr_point(rng, M=1.0, a=0.5):
    r = rng.uniform(2.1, 100.0)
    theta = rng.uniform(0.01, math.pi - 0.01)
    phi = rng.uniform(0.0, 2.0 * math.pi)
    t = rng.uniform(-1.0, 1.0)
    x = r * math.sin(theta) * math.cos(phi)
    y = r * math.sin(theta) * math.sin(phi)
    z = r * math.cos(theta)
    return [t, x, y, z], {"M": M, "a": a}


def _flrw_point(rng, t0=1.0):
    # FLRW is homogeneous; only t matters
    t = rng.uniform(0.1, 10.0)
    r = rng.uniform(0.0, 1.0)
    theta = rng.uniform(0.01, math.pi - 0.01)
    phi = rng.uniform(0.0, 2.0 * math.pi)
    return [t, r, theta, phi], {"t0": t0}


def _cghs_point(rng, M=1.0, sigma=0.0):
    # CGHS is 2D; x_plus and x_minus from point[0] and point[1]
    x_plus = rng.uniform(-1.0, 1.0)
    x_minus = rng.uniform(-1.0, 1.0)
    return [x_plus, x_minus, 0.0, 0.0], {"M": M, "sigma": sigma}


def _sym_args(metric, point):
    if metric["name"] == "schwarzschild":
        r = math.sqrt(point[1]**2 + point[2]**2 + point[3]**2)
        return (r,)
    elif metric["name"] == "kerr":
        r = math.sqrt(point[1]**2 + point[2]**2 + point[3]**2)
        theta = math.atan2(math.sqrt(point[1]**2 + point[2]**2), point[3])
        return (r, theta)
    elif metric["name"] == "flrw_matter":
        return (point[0],)
    elif metric["name"] == "cghs":
        return (point[0], point[1])
    return ()


if __name__ == "__main__":
    main()
