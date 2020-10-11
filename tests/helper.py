import sys, os, shutil, subprocess, re, signal, shlex
from pathlib import Path

def __error_status(code):
    if code and code < 0:
        try:
            return 'Process died with %r' % signal.Signals(-code)
        except ValueError:
            return 'Process died with unknown signal {}'.format(-code)
    else:
        return 'Process returned non-zero exit status {}'.format(code)

def __simc_cli_path(var):
    path = os.environ.get(var)
    if path is None:
        raise Exception("No SIMC_CLI_PATH environment variable set")
    path = shutil.which(path)
    if path is None:
        raise Exception("SIMC_CLI_PATH not valid")
    return str(Path(path).resolve())

IN_CI = 'CI' in os.environ

SIMC_CLI_PATH = __simc_cli_path('SIMC_CLI_PATH')
SIMC_ITERATIONS = int( os.environ.get('SIMC_ITERATIONS', '10') )
SIMC_THREADS = int( os.environ.get('SIMC_THREADS', '2') )
SIMC_FIGHT_STYLE = os.environ.get('SIMC_FIGHT_STYLE')
SIMC_PROFILE_DIR = os.environ.get('SIMC_PROFILE_DIR', os.getcwd())

def find_profiles(klass):
    files = Path(SIMC_PROFILE_DIR).glob('*_{}*.simc'.format(klass))
    return ( ( path.name, str(path.resolve()) ) for path in files )

class TestGroup(object):
    def __init__(self, name, **kwargs):
        self.name = name
        self.profile = kwargs.get('profile')
        self.fight_style = kwargs.get('fight_style', SIMC_FIGHT_STYLE)
        self.iterations = kwargs.get('iterations', SIMC_ITERATIONS)
        self.threads = kwargs.get('threads', SIMC_THREADS)
        self.tests = []

class Test(object):
    def __init__(self, name, **kwargs):
        self.name = name

        group = kwargs.get('group')
        if group:
            group.tests.append(self)

        self._profile = kwargs.get('profile', group and group.profile)
        self._fight_style = kwargs.get('fight_style', group and group.fight_style or SIMC_FIGHT_STYLE)
        self._iterations = kwargs.get('iterations', group and group.iterations or SIMC_ITERATIONS)
        self._threads = kwargs.get('threads', group and group.threads or SIMC_THREADS)
        self._args = kwargs.get('args', [])

    def args(self):
        args = [
            'iterations={}'.format(self._iterations),
            'threads={}'.format(self._threads),
            'cleanup_threads=1',
            'default_actions=1',
        ]
        if self._fight_style:
            args.append('fight_style={}'.format(self._fight_style))
        args.append(self._profile)
        for arg in self._args:
            if isinstance(arg, tuple):
                args.append(shlex.quote('{}={}'.format(*arg)))
            else:
                args.append(shlex.quote(str(arg)))
        return args

SIMC_WALL_SECONDS_RE = re.compile('WallSeconds\\s*=\\s*([0-9\\.]+)')
def run_test(test):
    args = [ SIMC_CLI_PATH ]
    args.extend(test.args())

    try:
        res = subprocess.run(args, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, encoding='UTF-8', timeout=30)
        wall_time = SIMC_WALL_SECONDS_RE.search(res.stdout)
        return ( True, float(wall_time.group(1)), None )
    except subprocess.CalledProcessError as err:
        return ( False, 0, err )

def run(tests):
    success = 0
    failure = 0
    total = 0

    def do_run(test):
        nonlocal total, failure, success
        total += 1
        print('  {:<60}    '.format(subtest.name), end='', flush=True)
        res, time, err = run_test(subtest)
        if res:
            print('[PASS] {:.5f}'.format(time))
            success += 1
        else:
            print('[FAIL]')
            print('-- {:<62} --------------'.format(__error_status(err.returncode)))
            print(err.cmd)
            if err.stderr:
                print(err.stderr.rstrip('\r\n'))
            print('-' * 80)
            failure += 1

    for test in tests:
        if isinstance(test, TestGroup):
            print(' {:<79}'.format(test.name))
            for subtest in test.tests:
                do_run(subtest)
        else:
            do_run(test)

    print('Passed: {}/{}'.format(success, total))

    return failure
