%% Using the wrapper:
sdk_path = "C:\\Program Files (x86)\\PCO Digital Camera Toolbox\\pco.sdk\\";
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

%% Setup camera
% You can also use pco.camware to set this manually and then
% use this script just for recording
c.reset_camera_settings();
c.set_framerate_exposure(1, 1e6, 1e6); % 1kHz, 1ms
c.arm_camera();

c.set_segment_sizes(500, 1000, 0, 0);

%% Record something in segment 1 but transfer later
set_active_segment(1);
c.start_recording();
if ~c.wait_for_recording_done(5000)
    c.stop_recording();
end

%% Record 10 bursts of 10 MIPs using segment 2
set_active_segment(2)
for i = 1:10
    c.clear_active_segment();

    c.start_recording();
    if ~c.wait_for_recording_done(5000)
        c.stop_recording();
    end

    % Transfer both MIP and full data. In practice you probably don't want to do that
    % Better just transfer all and perform MIP later. But I want to test both functions
    c.transfer_mip_to_tiff(2, 0, 100, 10, "mip.tiff");
    c.transfer_to_tiff(2, 0, 1000, "all.tiff");
end

%% Now transfer from segment 1
c.transfer_mip_to_tiff(1, 0, 100, 5, "mip.tiff");

%%
c.close()