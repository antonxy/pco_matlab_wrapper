%% Build definition
% Remove old definition
delete("definepco_wrapper.m");
delete("definepco_wrapper.mlx");
delete("pco_wrapperData.xml");

% Build library definition file
clibgen.generateLibraryDefinition(...
    "pco_wrapper.hpp",...
    "Libraries","C:\Users\fucko\AppData\Roaming\PCO Digital Camera Toolbox\pco.sdk\lib64\SC2_Cam.lib",...
    "PackageName","pco_wrapper",...
    "IncludePath", "C:\Users\fucko\AppData\Roaming\PCO Digital Camera Toolbox\pco.sdk\include"...
)

%% Build library
% Build dll from library definition
% Requires Visual Studio (2019?) to be installed.
% Maybe mingw also works?
build(definepco_wrapper);

%% Using the wrapper:
addpath pco_wrapper;
c = clib.pco_wrapper.PCOCamera(0);