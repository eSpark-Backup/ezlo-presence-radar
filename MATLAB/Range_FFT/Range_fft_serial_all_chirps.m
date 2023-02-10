close all
clear all
clc

% Serial port settings
COM = 'COM27'
BaudRate = 512000

% Close opened serial instances
if ~isempty(instrfind)
     fclose(instrfind);
     delete(instrfind);
end
delete(instrfind({'Port'},{COM}));

% Radar configurations
chirp_samples = 128

% Variables
chirp =1
mat = zeros(1,chirp_samples)
frameStartStr = chirp_samples % No. of samplesin a chirp as a reference for start of chirp

% Main Program
frameData = serial(COM,'BaudRate',BaudRate,'InputBufferSize',1024000)
fopen(frameData);

while true
    % Discard header 
    while true   
        if str2double(fgets(frameData)) == frameStartStr
            fgets(frameData) %discarding temperature data
            break
        end
    end

    for samples= 1:chirp_samples
        mat(samples) = str2double(fgets(frameData))
    end  
    w = window(@blackmanharris,chirp_samples);
    avg_windowed = (mat - mean(mat,2)).*w';
    zero_padded = [ avg_windowed zeros(size(avg_windowed)) ];
    
    range_fft = (fft(zero_padded'))/chirp_samples;
    range_fft = 2*range_fft.';
    range_fft = range_fft(:,1:chirp_samples);
    
    fft_abs = abs(range_fft);
    plot(fft_abs)
    title(['Chirp No. ',num2str(chirp)])
    drawnow
    
    %subplot(4,1,1)
    %plot(mat)


    %subplot(4,1,2)
    %plot(w)

    %subplot(4,1,3)
    %plot(avg_windowed)

    
    %subplot(4,1,4)
    %plot(zero_padded)

    
    %figure
    %plot(range_fft)

    
    %figure('Name','FFT ABS','NumberTitle','off');
    

    fft_avg = mean(fft_abs);   
    chirp = chirp+1
end

fclose(frameData);