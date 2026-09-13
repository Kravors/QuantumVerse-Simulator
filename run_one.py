#!/usr/bin/env python3
"""Line-buffered runner: prints each stdout line with a timestamp, waits on the
PROCESS (poll), not the pipe, so a real hang is distinguishable from a pipe hang."""
import subprocess, sys, time, os

exe = sys.argv[1] if len(sys.argv) > 1 else r"F:\syyyy\build\Debug\test_theory_discovery_agent.exe"
t0 = time.time()
try:
    p = subprocess.Popen([exe], stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                         bufsize=1, text=True, env={**os.environ, "PYTHONUNBUFFERED": "1"})
except FileNotFoundError:
    print("binary missing"); sys.exit(2)

last = ""
while True:
    line = p.stdout.readline()
    if line:
        last = line.rstrip()
        print(f"[{time.time()-t0:7.1f}s] {last}", flush=True)
        continue
    if p.poll() is not None:
        break
    time.sleep(0.05)

rc = p.wait()
print(f"\n=== process exited rc={rc} at {time.time()-t0:.1f}s ===")
print("LAST LINE:", last)