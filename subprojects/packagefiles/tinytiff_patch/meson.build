project('tinytiff', 'c')

tinytiff_inc = include_directories('./src')
tinytiff_src = ['./src/tinytiffwriter.c']
tinytiff = static_library('tinytiff', tinytiff_src, include_directories: tinytiff_inc)
tinytiff_dep = declare_dependency(link_with : tinytiff, include_directories : tinytiff_inc)