%% Using the wrapper:
setenv('PATH',[getenv('PATH') ';C:\Users\fucko\AppData\Roaming\PCO Digital Camera Toolbox\pco.sdk\bin64']);
addpath pco_wrapper;

% Unfortunately there is no helpful error message if the library can't be
% loaded, just "Unable to resolve the name clib.pco_wrapper.PCOCamera"
c = clib.pco_wrapper.PCOCamera();

% Opens the stdout console for matlab that is normally hidden
% the library can log some messages here
clib.pco_wrapper.openConsole();

c.open(0);
c.set_framerate_exposure(1, 1e6, 1e6); % 1kHz, 1ms
%c.set_segment_sizes([...]);
c.clear_active_segment();

% This is not accurate at all. Better record fixed number of frames.
% But I have not implemented that yet
c.start_recording();
pause(0.1); 
c.stop_recording();