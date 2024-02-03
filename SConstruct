import sys

AddOption('--clang', action='store_true', help='Force using Clang.')
if GetOption('clang'):
    env = Environment(TOOLS=['clang', 'clang++', 'gnulink'])
else:
    env = Environment()  # default toolchain

if 'msvc' in env['TOOLS']:
    # Flags for MSVC.
    env.Append(CCFLAGS=['/O2', '/W4', '/std:c++17', '/FS', '/EHsc'])
    env.Append(CPPDEFINES=['_CRT_SECURE_NO_WARNINGS'])
else:
    # Non-MSVC compilers tends to use these flags.
    env.Append(CCFLAGS=['-O3', '-Wall', '-Wextra', '-pedantic', '-std=c++17'])
if 'g++' in env['TOOLS'] or 'clang++' in env['TOOLS']:
    # This option is supported by GCC and Clang but probably not other compilers.
    env.Append(CCFLAGS=['-fdiagnostics-color=always'])
if 'clang++' in env['TOOLS']:
    # Clang doesn't conform to the standard by default. This fixes it.
    env.Append(CCFLAGS=['-frelaxed-template-template-args'])
# Commment this line to turn on assertions.
env.Append(CPPDEFINES=['NDEBUG'])
program = env.Program('karnaugh', env.Glob('./src/*.cc'))
env.Alias('build', program)

env.Replace(PYTHON_EXECUTABLE=sys.executable)
test = env.Alias('test', program, '${ESCAPE(PYTHON_EXECUTABLE)} tests/run.py --diff $SOURCE')
env.AlwaysBuild(test)
