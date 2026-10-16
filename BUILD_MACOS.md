# Building LINGOT on macOS

This guide explains how to compile and run LINGOT on macOS (Apple Silicon and Intel).

---

## 1. Prerequisites

Ensure you have the Xcode Command Line Tools and [Homebrew](https://brew.sh) installed:

```bash
xcode-select --install
```

### Install Dependencies via Homebrew

LINGOT requires GTK+ 3 for the user interface, FFTW for fast Fourier transforms, JSON-C for configuration parsing, and PulseAudio (or JACK) for audio input on macOS (as ALSA and OSS are Linux-only).

```bash
# Build system and translation utilities
brew install autoconf automake libtool pkg-config intltool gettext

# GUI and calculation libraries
brew install gtk+3 json-c fftw

# Audio backend
brew install pulseaudio
```

---

## 2. Environment Configuration

Homebrew's `gettext` is keg-only (not symlinked into the default `PATH`), so you must configure your environment before running the build scripts:

### Apple Silicon (M1/M2/M3/M4):
```bash
export PATH="/opt/homebrew/opt/gettext/bin:$PATH"
export PKG_CONFIG_PATH="/opt/homebrew/lib/pkgconfig:/opt/homebrew/opt/gettext/lib/pkgconfig:$PKG_CONFIG_PATH"
```

### Intel Macs:
```bash
export PATH="/usr/local/opt/gettext/bin:$PATH"
export PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:/usr/local/opt/gettext/lib/pkgconfig:$PKG_CONFIG_PATH"
```

---

## 3. Build Instructions

### 1. Generate Configuration Scripts
If building from Git:
```bash
./bootstrap
```

### 2. Configure
Configure the build disabling the Linux-only audio backends (`OSS` and `ALSA`) and enabling `PulseAudio`:

```bash
./configure --without-oss --without-alsa --with-pulseaudio
```

> **Note:** `--without-oss` is required on macOS because OSS is enabled by default in `./configure`, but macOS does not provide `<sys/soundcard.h>`.

### 3. Compile
```bash
make
```

### 4. (Optional) Install
To install globally into `/usr/local`:
```bash
sudo make install
```

---

## 4. Running LINGOT on macOS

LINGOT uses PulseAudio to capture microphone audio on macOS.

### Recommended (1-Step Launch): Enable Background Service
Run this command once to register PulseAudio as a persistent macOS background service:

```bash
brew services start pulseaudio
```

Once enabled, PulseAudio runs automatically whenever you log in. You can now launch Lingot anytime in **a single step**:

```bash
./lingot
# or if installed globally:
lingot
```

---

### Alternative: On-Demand Execution
If you prefer not to keep a background service running, you can launch PulseAudio and Lingot together in one command:

```bash
pulseaudio --start 2>/dev/null; ./lingot
```

> **Microphone Permissions:** The first time PulseAudio or your terminal accesses the microphone, macOS will prompt you for permission in **System Settings > Privacy & Security > Microphone**. Ensure permission is granted.

---

## 5. Troubleshooting & Technical Details

### Linker Error: `ld: unknown options: --as-needed`
Apple's Clang linker (`ld64`) does not support GNU `ld` specific flags such as `--as-needed`, `--no-allow-shlib-undefined`, and `--no-undefined`. These flags have been replaced with libtool's cross-platform `-no-undefined` in `Makefile.am`.

### Missing Audio Input
If LINGOT launches but does not respond to microphone input:
1. Check that the PulseAudio server is active:
   ```bash
   pulseaudio --check && echo "PulseAudio is running"
   ```
2. In LINGOT preferences (**Config** > **Audio Systems**), select **PulseAudio** as the active audio system.
