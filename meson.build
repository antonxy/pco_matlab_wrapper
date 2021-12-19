project('pco_matlab_wrapper', 'cpp')

pco_dir = 'C:\\Program Files (x86)\\PCO Digital Camera Toolbox\\pco.sdk\\'
pco_lib = 'SC2_Cam'

pco_dep = declare_dependency(
    link_args : ['-L' + pco_dir + 'lib64', '-l' + pco_lib],
    include_directories : include_directories(pco_dir + 'include'))

tinytiff_proj = subproject('tinytiff')
tinytiff_dep = tinytiff_proj.get_variable('tinytiff_dep')

executable('pco_transfer', 'pco_transfer.cpp', dependencies : [pco_dep, tinytiff_dep])