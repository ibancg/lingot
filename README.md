[![Build Status](https://travis-ci.com/ibancg/lingot.svg?branch=master)](https://travis-ci.com/ibancg/lingot)

# LINGOT - A musical instrument tuner.

<img align="right" src="http://lingot.nongnu.org/images/lingot-black-main.png">

LINGOT is a musical instrument tuner. It’s accurate, easy to use, and highly configurable. Originally conceived to tune electric
guitars, it can be used to tune other instruments.

It looks like an analogue tuner, with a gauge indicating the relative shift to a certain note,
determined automatically as the closest note to the estimated frequency.


## Main features

* It is free software, distributed under the [GPL license](https://www.gnu.org/licenses/old-licenses/gpl-2.0.html).
* Quick and accurate, perfect for real-time microtonal tuning.
* Easy to use. Just plug in your instrument and run it.
* _LINGOT_ is a universal tuner. It can tune many musical instruments, you only need to provide the scale _temperaments_. For that purpose, it supports the [.scl format](http://www.huygens-fokker.org/scala/scl_format.html) from the [Scala project](http://www.huygens-fokker.org/scala/).
* Configurable via GUI. It is possible to change any parameter while the program is running, without editing any file.

## Requirements

* A modest computer running _GNU/Linux_.
* A sound card with line-in or microphone input.
* [Jack](http://www.jackaudio.org/), [ALSA](https://www.alsa-project.org/main/index.php/Main_Page), [OSS](http://www.opensound.com/oss.html) or [PulseAudio](https://www.freedesktop.org/wiki/Software/PulseAudio/) support.
* [GTK+](https://www.gtk.org/) library, version _3.10_ or above.

## Installation

If you are building a release, just type:

```console
./configure
make install
```

If you are building a development version obtained from our VCS you will first need to
create the _configure_ script, by calling:

```console
./bootstrap
```

You can enable/disable the supported audio systems with the following options passed to the
_configure_ script, all of them enabled by default:

```
  --enable-oss=<yes|no>
  --enable-alsa=<yes|no>
  --enable-jack=<yes|no>
  --enable-pulseaudio=<yes|no>
```

Also, the depedency to [libfftw](http://fftw.org) can be enabled/disabled with

```
  --enable-libfftw=<yes|no>
```

Below a summary of the packages needed to build a development version on _Debian-based_ systems:

```console
sudo apt-get install \
    libtool \
    intltool \
    libasound2-dev \
    libpulse-dev \
    libjack-jackd2-dev \
    libfftw3-dev \
    libgtk-3-dev \
    libcunit1-dev
```

## Resources

* [Official LINGOT website](http://lingot.nongnu.org/)
* [Project page in Savannah](https://savannah.nongnu.org/projects/lingot/)
* [Downloads page in Savannah](https://savannah.nongnu.org/projects/lingot/)
