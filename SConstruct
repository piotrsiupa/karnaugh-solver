env = Environment()

env.Append(CCFLAGS=['-O3', '-Wall', '-Wextra', '-pedantic', '-std=c++17', '-fdiagnostics-color=always'])
env.Append(CPPDEFINES=['NDEBUG'])
program = env.Program('karnaugh', env.Glob('./src/*.cc'))
env.Alias('build', program)

test = env.Alias('test', program, 'tests/run.py $SOURCE')
env.AlwaysBuild(test)
