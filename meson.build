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

src = files(
    'src/ucp_context.cpp',
    'src/ucp_endpoint.cpp',
    'src/ucp_listener.cpp',
    'src/ucp_worker.cpp',
    'src/ucx_helper.cpp',
)

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
    [src, 'src/server.cpp'],
    dependencies: deps,
    include_directories: global_inc,
)

executable(
    'client',
    [src, 'src/client.cpp'],
    dependencies: deps,
    include_directories: global_inc,
)