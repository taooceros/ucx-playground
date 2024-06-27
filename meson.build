project('ucx', ['c', 'cpp'], default_options: ['cpp_std=c++20'])
ucx_dep = dependency('ucx')

add_project_arguments('-Werror=return-type', language: 'cpp')
add_project_arguments('-Werror=return-type', language: 'c')

executable('ucp_hello_world', 'ucp_hello_world/ucp_hello_world.c', dependencies: ucx_dep)
executable('ucp_client_server', 'ucp_hello_world/ucp_client_server.c', dependencies: ucx_dep)