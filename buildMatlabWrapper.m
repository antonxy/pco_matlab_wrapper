%% Build definition
sdk_path = "C:\Program Files (x86)\PCO Digital Camera Toolbox\pco.sdk\";
% Remove old definition
delete("definepco_wrapper.m");
delete("definepco_wrapper.mlx");
delete("pco_wrapperData.xml");

% Build library definition file
clibgen.generateLibraryDefinition(...
    "pco_wrapper.hpp",...
    "Libraries",fullfile(sdk_path, "lib64\SC2_Cam.lib"),...
    "PackageName","pco_wrapper",...
    "IncludePath", fullfile(sdk_path, "include")...
)

% Add extra definitions. This is designed in matlab to be done after
% building the defintion by manually editing the definition.
% But I don't like that, because then I can't easily rebuild the definition
% when I change something in the c++ file.
thedefine = definepco_wrapper;

ImageStackDefinition = find_class(thedefine.Classes, "ImageStack");

getDataDefinition = addMethod(ImageStackDefinition, ...
   "uint16_t const * ImageStack::getData(int num_images,int cols,int rows)", ...
   "Description", "clib.pco_wrapper.ImageStack.getData    Method of C++ class ImageStack"); % This description is shown as help to user. Modify it to appropriate description.
defineArgument(getDataDefinition, "num_images", "int32");
defineArgument(getDataDefinition, "cols", "int32");
defineArgument(getDataDefinition, "rows", "int32");
defineOutput(getDataDefinition, "RetVal", "uint16", ["num_images", "cols", "rows"]);
validate(getDataDefinition);

% PCOCameraDefinition = find_class(thedefine.Classes, "PCOCamera");
% 
% % This is not necessary in matlab 2020
% set_segment_sizesDefinition = addMethod(PCOCameraDefinition, ...
%     "void PCOCamera::set_segment_sizes(DWORD [4] pagesPerSegment)", ...
%     "Description", "clib.pco_wrapper.PCOCamera.set_segment_sizes    Method of C++ class PCOCamera"); % This description is shown as help to user. Modify it to appropriate description.
% defineArgument(set_segment_sizesDefinition, "pagesPerSegment", "uint32", "input", 4);
% validate(set_segment_sizesDefinition);

validate(thedefine);

disp("Added to definition")

%% Build library
% Build dll from library definition
% Requires Visual Studio (2019?) to be installed.
% Maybe mingw also works?
build(thedefine);


%%
function c = find_class(classes, name) 
    for i=1:size(classes)
        if classes(i).CPPName == name
            c = classes(i);
        end
    end
end
