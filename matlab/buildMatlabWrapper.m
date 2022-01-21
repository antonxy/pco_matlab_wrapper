%% Build definition
sdk_path = "C:\\Users\\fucko\\AppData\\Roaming\\PCO Digital Camera Toolbox\\pco.sdk\\";
% Remove old definition
delete("definepco_wrapper.m");
delete("definepco_wrapper.mlx");
delete("pco_wrapperData.xml");

% Build library definition file
clibgen.generateLibraryDefinition(...
    "../include/pco_wrapper.hpp",...
    "Libraries",[fullfile(sdk_path, "lib64\\SC2_Cam.lib"), "..\\builddir\\libpco_wrapper.a", "..\\builddir\\subprojects\\TinyTIFF-3.0.0.0\\libtinytiff.a"],...
    "PackageName","pco_wrapper",...
    "IncludePath", fullfile(sdk_path, "include")...
)

validate(definepco_wrapper);

%% Build library
% Build dll from library definition
% Requires Visual Studio (2019?) to be installed.
build(definepco_wrapper);