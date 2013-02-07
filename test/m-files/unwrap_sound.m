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
