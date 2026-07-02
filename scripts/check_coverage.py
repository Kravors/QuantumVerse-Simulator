#!/usr/bin/env python3
import sys
import subprocess
import re

def parse_lcov_coverage(info_file):
    total_lines = 0
    covered_lines = 0
    total_branches = 0
    covered_branches = 0
    
    with open(info_file, 'r') as f:
        for line in f:
            if line.startswith('LF:'):
                total_lines += int(line[3:].split(',')[0])
            elif line.startswith('LH:'):
                covered_lines += int(line[3:].split(',')[0])
            elif line.startswith('BRDA:'):
                parts = line[5:].split(',')
                if len(parts) >= 3:
                    total_branches += 1
            elif line.startswith('BRH:'):
                covered_branches += int(line[3:].split(',')[0])
    
    line_coverage = (covered_lines / total_lines * 100) if total_lines > 0 else 100
    branch_coverage = (covered_branches / total_branches * 100) if total_branches > 0 else 100
    
    return line_coverage, branch_coverage

def main():
    line_threshold = 95
    branch_threshold = 90
    
    try:
        line_cov, branch_cov = parse_lcov_coverage('coverage.info')
    except FileNotFoundError:
        print("No coverage.info found, skipping coverage check")
        return 0
    
    print(f"Line coverage: {line_cov:.1f}%")
    print(f"Branch coverage: {branch_cov:.1f}%")
    
    if line_cov < line_threshold or branch_cov < branch_threshold:
        print(f"Coverage below threshold: {line_threshold}% lines, {branch_threshold}% branches")
        return 1
    
    return 0

if __name__ == '__main__':
    sys.exit(main())