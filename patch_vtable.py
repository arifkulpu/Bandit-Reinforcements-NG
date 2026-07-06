import sys

file_path = r'C:\vcpkg\buildtrees\commonlibsse-ng\src\14c656992b-e31bd3db34.clean\include\RE\Offsets_VTABLE.h'
with open(file_path, 'r', encoding='utf-8') as f:
    lines = f.readlines()

new_lines = []
for line in lines:
    if line.startswith('\tinline std::array<REL::ID, ') and '{RELOCATION' in line:
        line = line.replace('{RELOCATION', '{{RELOCATION')
        line = line.replace('};', '};'.replace('};', '}};'))
    new_lines.append(line)

with open(file_path, 'w', encoding='utf-8') as f:
    f.writelines(new_lines)

print("Patched Offsets_VTABLE.h")
