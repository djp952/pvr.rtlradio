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
   
### Interface
> Configures Kodi interface settings   
   
| Setting | Description | Default |
| :-- | :-- | :--: |
| Prepend channel numbers to channel names | When set to ON the channel number will be prepended to the channel name when reported to Kodi. | __`OFF`__ |
   
### FM Radio
> Configures FM Radio settings   
   
| Setting | Description | Default |
| :-- | :-- | :--: |
| Radio Data System (RDS) region | Specifies the Radio Data System (RDS) region. When set to __`Automatic`__ the region will be automatically detected. When set to __`World`__ the global RDS standard will be used. When set to __`North America`__ the RBDS standard will be used. | __`Automatic`__ |
| PCM output sample rate | Specifies the FM digital signal processor PCM output sample rate. | __`48.0 KHz`__ |
   
> <sup>1</sup> Setting is available when __Connection type__ is set to __`Universal Serial Bus (USB)`__   
> <sup>2</sup> Setting is available when __Connection type__ is set to __`Network (rtl_tcp)`__   
