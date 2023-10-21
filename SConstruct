import sys

env = Environment()

if 'msvc' in env['TOOLS']:
    # Flags for MSVC
    env.Append(CCFLAGS=['/O2', '/W4', '/std:c++17', '/FS', '/EHsc'])
else:
    # Flags for GCC and Clang
    env.Append(CCFLAGS=['-O3', '-Wall', '-Wextra', '-pedantic', '-std=c++17', '-fdiagnostics-color=always'])
if 'clang++' in env['TOOLS']:
    # Clang doesn't conform to the standard by default. This fixes it.
    env.Append(CCFLAGS=['-frelaxed-template-template-args'])
env.Append(CPPDEFINES=['NDEBUG'])
program = env.Program('karnaugh', env.Glob('./src/*.cc'))
env.Alias('build', program)

env.Replace(PYTHON_EXECUTABLE=sys.executable)
test = env.Alias('test', program, '${ESCAPE(PYTHON_EXECUTABLE)} tests/run.py $SOURCE')
env.AlwaysBuild(test)
