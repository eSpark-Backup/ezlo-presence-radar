close all 
clear all
clc

csvread = dlmread('lomas.csv');
for frames = 1:36
    
    samples = csvread(frames,:)
    
    for chirps= 1:64 

        orgSamples = []
        start =0;
        chirp_samples = 128

        for num = 1:64
            orgSamples = [orgSamples ; samples(start+7: start+134)];
            start=  start + 134;
        end

        mat = orgSamples(chirps,:);
        w = window(@blackmanharris,chirp_samples);
        avg_windowed = (mat - mean(mat,2)).*w';

        %subplot(4,1,1)
        %plot(mat)


        %subplot(4,1,2)
        %plot(w)

        %subplot(4,1,3)
        %plot(avg_windowed)

        zero_padded = [ avg_windowed zeros(size(avg_windowed)) ];
        %subplot(4,1,4)
        %plot(zero_padded)

        range_fft = (fft(zero_padded'))/chirp_samples;
        range_fft = 2*range_fft.';
        range_fft = range_fft(:,1:chirp_samples);
        %figure
        %plot(range_fft)

        fft_abs = abs(range_fft);
        %figure('Name','FFT ABS','NumberTitle','off');
        plot(fft_abs)
        title(['Frame No. ',num2str(frames)])
        drawnow

        fft_avg = mean(fft_abs);
        fft_avg

        pause(1/10)
    end
    
end