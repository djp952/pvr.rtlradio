## Accessing the PVR Settings
* From the Kodi Add-ons browser, select __`My add-ons`__
* Select __`PVR clients`__
* Select __`RTL-SDR Radio PVR Client`__
* Select __`Configure`__

***
### Device
> Configures RTL-SDR device settings   
   
| Setting | Description | Default |
| :-- | :-- | :--: |
| Connection type | Specifies the RTL-SDR device connection type. When set to __`Universal Serial Bus (USB)`__, the device must be connected locally to this system. When set to __`Network (rtl_tcp)`__, the device must be attached to a system running the __rtl_tcp__ server application. | __`Universal Serial Bus (USB)`__ |
| Device index <sup>1</sup> | When multiple RTL-SDR devices are connected via Universal Serial Bus (USB), specifies the index of the device to connect to. If only one RTL-SDR device is connected, leave set to the default index __`0`__. | __`0`__ |
| rtl_tcp server address <sup>2</sup> | Specifies the IPv4 address of the __rtl_tcp__ server where the RTL-SDR device is connected. | __`NOT SPECIFIED`__ |
| rtl_tcp server port <sup>2</sup> | Specifies the port number that the __rtl_tcp__ server will be listening for client connections. If no port number was specified to __rtl_tcp__ leave set to the default port number __`1234`__. | __`1234`__ |
| Input sample rate | Specifies the input sample rate for the RTL-SDR device. Lower sample rates will improve system performance, whereas higher sample rates will improve audio quality. | __`1.6 MHz`__ |
| Frequency correction calibration value (PPM) | Specifies the frequency correction calibration offset to apply to the RTL-SDR device. If the calibration offset for the device is not known, leave set to the default value __`0`__. | __`0`__ |
   
### Interface
> Configures Kodi interface settings   
   
| Setting | Description | Default |
| :-- | :-- | :--: |
| Prepend channel numbers to channel names | When set to ON the channel number will be prepended to the channel name when reported to Kodi. | __`OFF`__ |
   
### FM Radio
> Configures FM Radio settings   
   
| Setting | Description | Default |
| :-- | :-- | :--: |
| Enable Radio Data System (RDS) | When set to __`ON`__ detected Radio Data System (RDS) data embedded in the FM signal will be decoded and processed. | __`ON`__ |
| Radio Data System (RDS) region <sup>3</sup> | Specifies the Radio Data System (RDS) region. When set to __`Automatic`__ the region will be automatically detected. When set to __`World`__ the global RDS standard will be used. When set to __`North America`__ the RBDS standard will be used. | __`Automatic`__ |
| Downsample quality | Specifies the FM Digital Signal Processor (DSP) downsample quality. When set to __`Fast`__, downsampling will be optimized for system performance. When set to __`Maximum`__, downsampling will be optimized for audio quality. | __`Standard`__ |
| PCM output sample rate | Specifies the FM digital signal processor PCM output sample rate. | __`48.0 KHz`__ |
| PCM output gain | Specifies the FM Digital Signal Processor (DSP) PCM output audio gain. Lower gain values will reduce the perceived volume of the audio, whereas higher gain values will increase the perceived volume of the audio. | __`-3.0 dB`__ |
   
> <sup>1</sup> Setting is available when __Connection type__ is set to __`Universal Serial Bus (USB)`__   
> <sup>2</sup> Setting is available when __Connection type__ is set to __`Network (rtl_tcp)`__   
> <sup>3</sup> Setting is available when __Enable Radio Data System (RDS)__ is set to __`ON`__   
