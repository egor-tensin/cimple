add_project_arguments('-D_GNU_SOURCE', language: 'c')

git2 = dependency('libgit2')
json_c = dependency('json-c')
sodium = dependency('libsodium')
sqlite3 = dependency('sqlite3')
threads = dependency('threads')

rev = run_command('git', 'rev-parse', '--verify', 'HEAD', check: true)

const_c_config = configuration_data(
    {
        'PROJECT_VERSION': meson.project_version(),
        'PROJECT_REV': rev.stdout().strip(),
    },
)

foreach opt : ['DEFAULT_HOST', 'DEFAULT_PORT', 'DEFAULT_SQLITE_PATH']
    const_c_config.set(opt, get_option(opt))
endforeach

const_c = configure_file(
    input: 'const.c.template',
    output: 'const.c',
    configuration: const_c_config,
)

subdir('sqlite')

server = executable(
    meson.project_name() + '-server',
    sources: [
        'base64.c',
        'buf.c',
        'cmd_line.c',
        'command.c',
        const_c,
        'event_loop.c',
        'file.c',
        'json.c',
        'json_rpc.c',
        'log.c',
        'net.c',
        'process.c',
        'protocol.c',
        'run_queue.c',
        'signal.c',
        'server.c',
        'server_main.c',
        'sqlite.c',
        sqlite_sql_h,
        'storage.c',
        'storage_sqlite.c',
        'string.c',
        'tcp_server.c',
        'worker_queue.c',
    ],
    dependencies: [
        json_c,
        sodium,
        sqlite3,
        threads,
    ],
    implicit_include_directories: false,
    include_directories: include_directories('sqlite'),
)

client = executable(
    meson.project_name() + '-client',
    sources: [
        'base64.c',
        'buf.c',
        'client.c',
        'client_main.c',
        'cmd_line.c',
        const_c,
        'file.c',
        'json.c',
        'json_rpc.c',
        'log.c',
        'net.c',
        'process.c',
        'protocol.c',
        'run_queue.c',
    ],
    dependencies: [
        json_c,
        sodium,
    ],
    implicit_include_directories: false,
)

worker = executable(
    meson.project_name() + '-worker',
    sources: [
        'base64.c',
        'buf.c',
        'ci.c',
        'cmd_line.c',
        'command.c',
        const_c,
        'event_loop.c',
        'file.c',
        'git.c',
        'json.c',
        'json_rpc.c',
        'log.c',
        'net.c',
        'process.c',
        'protocol.c',
        'run_queue.c',
        'signal.c',
        'string.c',
        'worker.c',
        'worker_main.c',
    ],
    dependencies: [
        git2,
        json_c,
        sodium,
    ],
    implicit_include_directories: false,
)
