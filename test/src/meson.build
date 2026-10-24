test_args = [
    '-m',
    'pytest',
    '--no-header',
    '-v',
    '--durations',
    '0',
    '--durations-min',
    '1.0',
    meson.current_source_dir(),
    '--server',
    server.full_path(),
    '--worker',
    worker.full_path(),
    '--client',
    client.full_path(),
    '--sigsegv',
    sigsegv.full_path(),
    '--project-version',
    meson.project_version(),
]

python = import('python').find_installation()

test('default', python, args: test_args, timeout: 600)
