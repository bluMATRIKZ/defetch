# litefetch
neofetch clone that is super light, should compile for old cpus and distros, and is portable

```
OS:             Ubuntu 24.04.2 LTS
Host:           OptiPlex 790
Kernel:         6.11.0-26-generic
Uptime:         16 hours, 6 mins
Shell:          bash bash,
Packages:       2598 (dpkg), 19 (flatpak)
Resolution:     1440x900
DE:             LXQt
Terminal:       bash
CPU:            Intel(R) Core(TM) i5-2400 CPU @ 3.10GHz
GPU:            Intel Corporation 2nd Generation Core Processor Family Integrated Graphics Controller (rev 09)
Memory:         2924 MiB / 7822 MiB
```

Compile & install:
```
make
```
if you have problems with make:
```
make old
```

Compile:
```
make compile
```
if you have problems with make compile:
```
make old-compile
```

Install:
```
make install
```

Uninstall:
```
make remove
```

Clean:
```
make clean
```
Run:
```
litefetch
```
or if you did not install it:
```
./litefetch
```
