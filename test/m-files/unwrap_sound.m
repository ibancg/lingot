fs = 44100;
T = 1/fs;

x = load('/tmp/dump.txt');
x2 = unwrap(x/32768*pi)*32768/pi;

t = (0:length(x)-1)*T;
f = (0:length(x)-1)*(fs/length(x));

t0 = 1;
t1 = length(t);
figure(1);
plot(t(t0:t1), x(t0:t1), 'b.-');
hold on;
plot(t(t0:t1), x2(t0:t1), 'r--');
hold off;
grid on;
xlabel('t');
ylabel('x');

X = fft(x);
X2 = fft(x2);

f0 = 1;
f1 = 8000;
figure(2);
plot(f(f0:f1), 10*log10(abs(X(f0:f1))), 'b-');
hold on;
plot(f(f0:f1), 10*log10(abs(X2(f0:f1))), 'r--');
hold off;
grid on;
xlabel('f [Hz]');
ylabel('10.log_{10}|x|');

