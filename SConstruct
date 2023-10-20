import sys

env = Environment()

if 'msvc' in env['TOOLS']:
    env.Append(CCFLAGS=['/O2', '/W4', '/std:c++17', '/FS', '/EHsc'])
else:
    env.Append(CCFLAGS=['-O3', '-Wall', '-Wextra', '-pedantic', '-std=c++17', '-fdiagnostics-color=always'])
env.Append(CPPDEFINES=['NDEBUG'])
program = env.Program('karnaugh', env.Glob('./src/*.cc'))
env.Alias('build', program)

env.Replace(PYTHON_EXECUTABLE=sys.executable)
test = env.Alias('test', program, '${ESCAPE(PYTHON_EXECUTABLE)} tests/run.py $SOURCE')
env.AlwaysBuild(test)
