close all
clear all
clc

% Graph Settings
num_of_fig = 3;

% Serial port settings
COM = 'COM27';
BaudRate = 512000;

% Close opened serial instances
if ~isempty(instrfind)
     fclose(instrfind);
     delete(instrfind);
end
delete(instrfind({'Port'},{COM}));

% Radar configurations
chirp_samples = 128;
No_of_chirps = 64;

% Variables
frame =1;
mat = zeros(No_of_chirps,chirp_samples);
fft_abs = zeros(No_of_chirps,chirp_samples);
fft_abs_tp = transpose(fft_abs);
coh_int_tp = zeros(No_of_chirps,1);
coh_int = transpose(coh_int_tp);
mti = coh_int;

frameStartStr = chirp_samples; % No. of samplesin a chirp as a reference for start of chirp

% Main Program
frameData = serial(COM,'BaudRate',BaudRate,'InputBufferSize',1024000);
fopen(frameData);

while true
    % Get Frame
    for chirps = 1:No_of_chirps
        % Discard header 
        while true   
            if str2double(fgets(frameData)) == frameStartStr
                fgets(frameData); %discarding temperature data
                break
            end
        end
        % Frame Data
        for samples= 1:chirp_samples
            mat(chirps,samples) = str2double(fgets(frameData));
        end 
    end
    
    % Range FFT
    w = window(@blackmanharris,chirp_samples);
    for chirps = 1:No_of_chirps
        avg_windowed = (mat(chirps,:) - mean(mat(chirps,:),2)).*w';
        zero_padded = [ avg_windowed zeros(size(avg_windowed)) ];

        range_fft = (fft(zero_padded'))/chirp_samples;
        range_fft = 2*range_fft.';
        range_fft = range_fft(:,1:chirp_samples);
        fft_abs(chirps,:) = abs(range_fft);
    end
    
    % Coherent Integration
    fft_abs_tp = transpose(fft_abs);
    coh_int_tp = pulsint(fft_abs_tp,"coherent");
    coh_int = transpose(coh_int_tp);
    
        
    % Moving Target Indicator
    n = 7;   % filter order
    mti = filter(ones(n, 1)/n, 1, coh_int);
    
    % Spatial Filtering
    spat_filt_w = gausswin(10, 2.5);
    spat_filt_w = spat_filt_w/sum(spat_filt_w);
    spat_filt = filter(spat_filt_w, 1, mti);
 
    % Graphs
    subplot(num_of_fig,1,1);
    plot(coh_int);
    xlabel('Coherent Integration');
    title(['Frame No. ',num2str(frame)])
    
    subplot(num_of_fig,1,2);
    plot(mti);
    xlabel('Moving Average Filter(MTI)');
    
    subplot(num_of_fig,1,3);
    plot(spat_filt);
    xlabel('Spatial Filtering');
    drawnow  
    

    %fft_avg = mean(fft_abs);   
    frame = frame+1
    flushinput(frameData);
end

fclose(frameData);