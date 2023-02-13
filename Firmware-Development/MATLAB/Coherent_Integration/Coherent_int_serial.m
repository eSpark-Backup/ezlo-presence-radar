close all
clear all
clc

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

frameStartStr = chirp_samples; % No. of samplesin a chirp as a reference for start of chirp

% Main Program
frameData = serial(COM,'BaudRate',BaudRate,'InputBufferSize',1024000);
fopen(frameData);

while true
    % Discard header 
    for chirps = 1:No_of_chirps
        while true   
            if str2double(fgets(frameData)) == frameStartStr
                fgets(frameData); %discarding temperature data
                break
            end
        end

        for samples= 1:chirp_samples
            mat(chirps,samples) = str2double(fgets(frameData));
        end 
    end
    
    w = window(@blackmanharris,chirp_samples);
    for chirps = 1:No_of_chirps
        avg_windowed = (mat(chirps,:) - mean(mat(chirps,:),2)).*w';
        zero_padded = [ avg_windowed zeros(size(avg_windowed)) ];

        range_fft = (fft(zero_padded'))/chirp_samples;
        range_fft = 2*range_fft.';
        range_fft = range_fft(:,1:chirp_samples);
        fft_abs(chirps,:) = abs(range_fft);
    end
    
    fft_abs_tp = transpose(fft_abs);
    coh_int_tp = pulsint(fft_abs_tp,"coherent");
    coh_int = transpose(coh_int_tp);
    plot(coh_int);
    title(['Frame No. ',num2str(frame)])
    drawnow  

    fft_avg = mean(fft_abs);   
    frame = frame+1
    flushinput(frameData);
end

fclose(frameData);