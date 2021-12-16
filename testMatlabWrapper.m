%% Using the wrapper:
sdk_path = "C:\Program Files (x86)\PCO Digital Camera Toolbox\pco.sdk\";
setenv('PATH',strcat(getenv('PATH'), ";", fullfile(sdk_path, "bin64")));
addpath pco_wrapper;

%% 
% Unfortunately there is no helpful error message if the library can't be
% loaded, just "Unable to resolve the name clib.pco_wrapper.PCOCamera"
% If you get this error check that the path above is set correctly
% (pointing to the bin64 folder of the pco sdk)
c = clib.pco_wrapper.PCOCamera();

% Opens the stdout console for matlab that is normally hidden
% the library can log some messages here
clib.pco_wrapper.openConsole();

c.open(0);

%% 
c.reset_camera_settings();
c.set_framerate_exposure(1, 1e6, 1e6); % 1kHz, 1ms
%c.set_segment_sizes([...]);
c.clear_active_segment();

c.arm_camera();
%% 
% This is not accurate at all. Better record fixed number of frames.
% But I have not implemented that yet
c.start_recording();
pause(1); 
c.stop_recording();

%%
image = c.transfer_mip(1, 1, 100, 10);

%%
c.close()

%%
% To get the image matrix from the ImageStack object
% you have to pass it's size to the getData function.
% This is a workaround for a limitation in the matlab c++ interface.
% I think there is no other way.
im = image.getData(image.num_images, image.cols, image.rows);
figure;
imagesc(squeeze(im(5, :, :)));