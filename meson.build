project('pco_matlab_wrapper', 'cpp')

pco_dir = 'C:\\Program Files (x86)\\PCO Digital Camera Toolbox\\pco.sdk\\'
pco_lib = 'SC2_Cam'

pco_dep = declare_dependency(
    link_args : ['-L' + pco_dir + 'lib64', '-l' + pco_lib],
    include_directories : include_directories(pco_dir + 'include')
)

tinytiff_proj = subproject('tinytiff')
tinytiff_dep = tinytiff_proj.get_variable('tinytiff_dep')

pco_wrapper_inc = include_directories('./include')
pco_wrapper = static_library('pco_wrapper', 'src/pco_wrapper.cpp', include_directories: pco_wrapper_inc, dependencies : [pco_dep, tinytiff_dep])
pco_wrapper_dep = declare_dependency(link_with : pco_wrapper, include_directories : pco_wrapper_inc)

executable('pco_transfer', 'src/pco_transfer.cpp', dependencies : [pco_dep, tinytiff_dep, pco_wrapper_dep])

test_with_camera = executable('test_with_camera', 'src/test_with_camera.cpp', dependencies : [pco_dep, tinytiff_dep, pco_wrapper_dep])
test('Test with camera', test_with_camera)