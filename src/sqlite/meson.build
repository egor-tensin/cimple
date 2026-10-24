python = import('python').find_installation()

sqlite_sql_h = custom_target(
    'sqlite_sql_h',
    input: ['../generate-sql-header.py', 'v01.sql'],
    output: 'sqlite_sql.h',
    command: [python, '@INPUT0@', meson.current_source_dir(), '-o', '@OUTPUT@'],
)
