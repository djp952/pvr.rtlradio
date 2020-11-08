## RTL-SDR Toolkit
As part of the PVR, platform-specific versions of the OSMOCOM RTL-SDR command line tools and a version of the SQLite command line tool are also provided.

### RTL-SDR Command Line Tools

[Formal Documentation](https://osmocom.org/projects/rtl-sdr/wiki/Rtl-sdr#Software) (https://osmocom.org/projects/rtl-sdr/wiki/Rtl-sdr#Software)

| Tool | Description |
| :-- | :-- |
| __rtl_adsb__ <sup>1</sup>| Simple ADS-B decoder.<br>_Uses a re-purposed DVB-T receiver as a software defined radio to receive and decode ADS-B data._ |
| __rtl_biast__ | Tool for turning the RTL-SDR.com bias tee or any GPIO ON and OFF|
| __rtl_eeprom__ | EEPROM programming tool for RTL2832 based DVB-T receivers.<br>_Dumps configuration and also writes EEPROM configuration._ |
| __rtl_fm__ | Simple FM demodulator for RTL2832 based DVB-T receivers.<br>_Uses a re-purposed DVB-T receiver as a software defined radio to receive narrow band FM signals and demodulate to audio._ |
| __rtl_power__ <sup>2</sup>| Simple FFT logger for RTL2832 based DVB-T receivers.<br>_This tool gathers signal data over a very wide area of the frequency spectrum, and then that data can be used to find active areas of the spectrum._ |
| __rtl_sdr__ | I/Q recorder for RTL2832 based DVB-T receivers.<br>_Uses a re-purposed DVB-T receiver as a software defined radio to receive signals in I/Q data form._ |
| __rtl_tcp__ <sup>3    </sup>| I/Q spectrum server for RTL2832 based DVB-T receivers.<br>_Uses a re-purposed DVB-T receiver as a software defined radio to receive and send I/Q data via TCP network to another demodulation, decoding or logging application._ |
| __rtl_test__ | Benchmark tool for RTL2832 based DVB-T receivers.<br>_Test tuning range and functional sample rates of your device on your system. Uses a re-purposed DVB-T receiver as a software defined radio._ |

> <sup>1</sup> The __rtl_adsb__ tool is not provided for Android platforms due to an incompatibility with the Android APK currently used by the build environment.   

> <sup>2</sup> The __rtl_power__ tool has been modified for proper operation on Windows 64-bit (x64) platforms.   

> <sup>3</sup> The __rtl_tcp__ tool has been modified for proper operation on Windows platforms, and incorporates proposed performance enhancements by __blinick__ (https://github.com/blinick/rtl-sdr/tree/wip_rtltcp_ringbuf).   
***
### SQLite Command Line Tool

[Formal Documentation](https://sqlite.org/cli.html) (https://sqlite.org/cli.html)

| Tool | Description |
| :-- | :-- |
| __sqlite3__ | Provides a simple command-line program that allows the user to manually enter and execute SQL statements against a SQLite database. |


