#!/usr/bin/env python3
import re

with open('Makefile', 'r') as f:
    lines = f.readlines()

fixed_lines = []
for i, line in enumerate(lines, 1):
    # If line starts with spaces (8 or more) followed by something, it's likely a recipe line
    if re.match(r'^ {8,}', line) and not line.strip().startswith('#'):
        # Replace leading spaces with a tab
        stripped = line.lstrip(' ')
        fixed_line = '\t' + stripped
        fixed_lines.append(fixed_line)
    else:
        fixed_lines.append(line)

with open('Makefile', 'w') as f:
    f.writelines(fixed_lines)

print("Fixed tab issues in Makefile")
