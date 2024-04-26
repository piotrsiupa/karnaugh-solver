import os
import sys

AddOption('--clang', action='store_true', help='Force using Clang.')
if GetOption('clang'):
    env = Environment(TOOLS=['clang', 'clang++', 'gnulink'])
else:
    env = Environment()  # default toolchain

env.Replace(CPP_STANDARD='c++20')
if 'msvc' in env['TOOLS']:
    # Flags for MSVC.
    env.Append(CCFLAGS=['/O2', '/W4', '/std:$CPP_STANDARD', '/FS', '/EHsc', '/utf-8', '/Zc:threadSafeInit-'])
    env.Append(CPPDEFINES=['_CRT_SECURE_NO_WARNINGS'])
else:
    # Non-MSVC compilers tends to use these flags.
    env.Append(CCFLAGS=['-O3', '-Wall', '-Wextra', '-pedantic', '-std=$CPP_STANDARD'])
if 'g++' in env['TOOLS'] or 'clang++' in env['TOOLS']:
    # These options are supported by GCC and Clang but probably not other compilers.
    env.Append(CCFLAGS=['-fdiagnostics-color=always', '-fno-threadsafe-statics'])
if 'clang++' in env['TOOLS']:
    # Clang doesn't conform to the standard by default. This fixes it.
    env.Append(CCFLAGS=['-frelaxed-template-template-args'])

AddOption('--dev', action='store_true', help='Development build. (assertions and warnings as errors)')
if GetOption('dev'):
    if 'msvc' in env['TOOLS']:
        env.Append(CCFLAGS=['/WX'])
    else:
        env.Append(CCFLAGS=['-Werror'])
else:
    # Turn off assertions.
    env.Append(CPPDEFINES=['NDEBUG'])
    # Set multithreaded build as default.
    SetOption('num_jobs', os.cpu_count() or 1)

program = env.Program('karnaugh', env.Glob('./src/*.cc'))
env.Alias('build', program)

env.Replace(PYTHON_EXECUTABLE=sys.executable)
test = env.Alias('test', program, '${ESCAPE(PYTHON_EXECUTABLE)} tests/run.py --diff $SOURCE')
env.AlwaysBuild(test)
