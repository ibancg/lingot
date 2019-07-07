[![Build Status](https://travis-ci.com/ibancg/lingot.svg?branch=master)](https://travis-ci.com/ibancg/lingot)

# LINGOT - A musical instrument tuner.

LINGOT is a musical instrument tuner. It’s accurate, easy to use, and highly configurable. Originally conceived to tune electric
guitars, it can be used to tune other instruments.

It looks like an analogue tuner, with a gauge indicating the relative shift to a certain note,
determined automatically as the closest note to the estimated frequency.

[![Main window](http://lingot.nongnu.org/images/lingot-black-main.png)](http://lingot.nongnu.org/images/lingot-black-main.png)

## Main features

* It’s free software. LINGOT is distributed under the [GPL license](https://www.gnu.org/licenses/old-licenses/gpl-2.0.html).
* It’s really quick and accurate, perfect for real-time microtonal tuning.
* Easy to use. Just plug in your instrument and run it.
* LINGOT is a universal tuner. It can tune many musical instruments, you only need to provide the temperaments. For that purpose, it supports the [.scl format](http://www.huygens-fokker.org/scala/scl_format.html) from the [Scala project](http://www.huygens-fokker.org/scala/).
* Configurable via GUI. It’s possible to change any parameter while the program is running, without editing any file.

## Requirements

* Sound card.
* Linux kernel with audio support (OSS, ALSA, JACK or PulseAudio).
* GTK+ library, version 3.10.

# Installation

If you are building a release, just type:

```console
./configure
make install
```

If you are building a development version obtained from our VCS you will need to
create the configure script first by calling:

```console
./bootstrap
```


Lingot supports the following audio systems/servers:

* OSS
* ALSA
* JACK
* PulseAudio

You can enable/disable each of them with the following options passed to the
configure script, all of them enabled by default:

```
  --enable-oss=<yes|no>
  --enable-alsa=<yes|no>
  --enable-jack=<yes|no>
  --enable-pulseaudio=<yes|no>
```

Also, the depedency to libfftw can be enabled/disabled with

```
  --enable-libfftw=<yes|no>
```

Please, see the INSTALL file.


# Synopsis

```
    lingot [-c config]
```

The `-c` option causes the search of a file named `{config}.conf` in the `~/.config/lingot`
folder. For example:

```
    lingot -c bass
```

will take the configuration file `~/.config/lingot/bass.conf`. This is useful for
maintaining different configurations for different instruments. It's also
possible to load and save configuration files from the GUI. The default
configuration file is `~/.config/lingot/lingot.conf`.
