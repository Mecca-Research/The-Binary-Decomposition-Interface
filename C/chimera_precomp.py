# Chimera Precompiler Layer (PRECOMP)
import re
import sys


# Very simple static annotation of a C file


def annotate_binary_literals(code):
pattern = re.compile(r'(\d+)')
def to_bin(match):
val = int(match.group(0))
return f"{match.group(0)} /* binary: {val:032b} */"
return pattern.sub(to_bin, code)


def process_file(filename):
with open(filename, 'r') as f:
source = f.read()
annotated = annotate_binary_literals(source)
output = filename.replace(".chc", ".c")
with open(output, 'w') as f:
f.write(annotated)
print(f"Annotated source written to {output}")


if __name__ == '__main__':
if len(sys.argv) != 2:
print("Usage: python chimera_precomp.py <source.chc>")
else:
process_file(sys.argv[1])
