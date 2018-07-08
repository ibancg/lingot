%
% lingot, a musical instrument tuner.
%
% Copyright (C) 2013  Iban Cereijo
%
% This file is part of lingot.
%
% lingot is free software; you can redistribute it and/or modify
% it under the terms of the GNU General Public License as published by
% the Free Software Foundation; either version 2 of the License, or
% (at your option) any later version.
%
% lingot is distributed in the hope that it will be useful,
% but WITHOUT ANY WARRANTY; without even the implied warranty of
% MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
% GNU General Public License for more details.
%
% You should have received a copy of the GNU General Public License
% along with lingot; if not, write to the Free Software
% Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA



%
% This file contains some signal processing functions in Octave in order
% to debug signals captured with Lingot.
%
% This file is not intended to be part of the unit tests nor is it intended
% to be distributed.
%


% representable pieces
delta_t = 0.02;
delta_f = 1800;


fs = 44100;
T = 1/fs;

x = load('/tmp/dump_pre_filter.txt');
xu = unwrap(x/32768*pi)*32768/pi;

t = (0:length(x)-1)*T;
f = (0:length(x)-1)*(fs/length(x));

t0 = 1;
t1 = round(delta_t/T);
figure(1);
plot(t(t0:t1), xu(t0:t1), 'r.-');
hold on;
plot(t(t0:t1), x(t0:t1), 'b--');
hold off;
grid on;
legend('unwrapped', 'original');
xlabel('t [s]');
ylabel('x');
title('before filter');
print -dpng /tmp/pic_before_filter_time.png
X = fft(x);
Xu = fft(xu);

f0 = 1;
f1 = round((delta_f/fs)*length(f));
figure(2);
plot(f(f0:f1), 10*log10(abs(Xu(f0:f1))), 'r--');
hold on;
plot(f(f0:f1), 10*log10(abs(X(f0:f1))), 'b-');
hold off;
grid on;
legend('unwrapped', 'original');
xlabel('f [Hz]');
ylabel('10.log_{10}|x|');
title('before filter');
print -dpng /tmp/pic_before_filter_freq.png


% ------------------------------------

oversampling = 1;
fs = 44100/oversampling;
T = 1/fs;

x = load('/tmp/dump_post_filter.txt');
xu = unwrap(x/32768*pi)*32768/pi;

t = (0:length(x)-1)*T;
f = (0:length(x)-1)*(fs/length(x));

t0 = 1;
t1 = round(delta_t/T);
figure(3);
plot(t(t0:t1), x(t0:t1), 'b--');
grid on;
xlabel('t [s]');
ylabel('x');
title('after filter');
print -dpng /tmp/pic_after_filter_time.png

X = fft(x);
Xu = fft(xu);

f0 = 1;
f1 = round((delta_f/fs)*length(f));
figure(4);
plot(f(f0:f1), 10*log10(abs(X(f0:f1))), 'b-');
grid on;
xlabel('f [Hz]');
ylabel('10.log_{10}|x|');
title('after filter');
print -dpng /tmp/pic_after_filter_freq.png

% ------------------------------------

oversampling = 23;
fs = 44100/oversampling;
T = 1/fs;

x = load('/tmp/dump_post.txt');
xu = unwrap(x/32768*pi)*32768/pi;

t = (0:length(x)-1)*T;
f = (0:length(x)-1)*(fs/length(x));

t0 = 1;
t1 = round(delta_t/T);
figure(5);
plot(t(t0:t1), x(t0:t1), 'b--');
grid on;
xlabel('t [s]');
ylabel('x');
title('after decimtaion');
print -dpng /tmp/pic_after_decimation_time.png

X = fft(x);
Xu = fft(xu);

f0 = 1;
f1 = round((delta_f/fs)*length(f));
figure(6);
plot(f(f0:f1), 10*log10(abs(X(f0:f1))), 'b-');
grid on;
xlabel('f [Hz]');
ylabel('10.log_{10}|x|');
title('after decimation');
print -dpng /tmp/pic_after_decimation_freq.png
