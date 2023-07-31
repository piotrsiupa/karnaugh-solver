env = Environment()

env.Append(CCFLAGS=['-O3', '-Wall', '-Wextra', '-pedantic', '-fdiagnostics-color=always'])
program = env.Program('karnaugh', env.Glob('./src/*.cc'))
env.Alias('build', program)

test = env.Alias('test', program, 'tests/run.py $SOURCE')
env.AlwaysBuild(test)
