project('ucx', ['c', 'cpp'], default_options: ['cpp_std=c++23'])

ucx_dep = dependency('ucx')

add_project_arguments('-Werror=return-type', language: 'cpp')
add_project_arguments('-Werror=return-type', language: 'c')

global_inc = include_directories(['include'])

cpptrace_dep = dependency('cpptrace')
libdwarf = dependency('libdwarf')
libzstd = dependency('libzstd')
libz = dependency('zlib')

fmt_proj = subproject('fmt')
fmt_dep = fmt_proj.get_variable('fmt_dep')

deps = [ucx_dep, cpptrace_dep, libdwarf, libzstd, libz, fmt_dep]

helper_src = files('src/ucx_helper.cpp', 'src/ucp_listener.cpp')

executable(
    'ucp_hello_world',
    'ucp_hello_world/ucp_hello_world.c',
    dependencies: ucx_dep,
)
executable(
    'ucp_client_server',
    'ucp_hello_world/ucp_client_server.c',
    dependencies: ucx_dep,
)

executable(
    'server',
    [helper_src, 'src/server.cpp'],
    dependencies: deps,
    include_directories: global_inc,
)

executable(
    'client',
    [helper_src, 'src/client.cpp'],
    dependencies: deps,
    include_directories: global_inc,
)