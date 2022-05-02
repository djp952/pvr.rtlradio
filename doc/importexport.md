## Importing and Exporting Channel Data
* From the Kodi Settings, select __`PVR & Live TV`__
* Select __`Client specific`__
* Select __`Client specific settings`__

### Import channel data
Selecting this client specific setting will open a file browser dialog that allows for selection of an existing __.json__ document to import into the PVR database. The file must adhere to the __JSON Schema__ described below.

Channel data imported into the PVR database with this method will add or replace matching entries but will not remove any non-matching entries. To completely replace the contents of the database, first execute the __Clear channel data__ operation followed by the __Import channel data__ operation.

### Export channel data
Selecting this client specific setting will open a folder browser dialog allows for selection of the folder to write the __radiochannels.json__ file. This file will contain the contents of the PVR database formatted to the __JSON Schema__ described below. Due to limitations the file name cannot be customized and will be set to __radiochannels.json__.

### Clear channel data
Selecting this client specific setting will clear all existing channel data from the PVR database.

### Hybrid Digital (HD) Radio channels (North America)
[Hybrid Digital (HD) Radio](https://hdradio.com/) channels are supported by the PVR and may be included as part of the channel import. The PVR allows for both the analog and digital signals to coexist by using separate channel numbers. Wideband FM Radio channels will be numbered by the frequency specified in Megahertz (101.9, for example), whereas Hybrid Digital (HD) Radio channels will be numbered by the frequency specified in 100 Kilohertz (1019, for example).

### Weather Radio channels (North America)
[NOAA Weather Radio (US)](https://www.weather.gov/nwr&ln_desc=NOAA+Weather+Radio/), [Weatheradio (Canada)](https://www.canada.ca/en/environment-climate-change/services/weatheradio.html), and SARMEX (Mexico) channels are supported by the PVR and may be included as part of the channel import. Weather Radio channels must have a specified frequency between 162400000 (162.400MHz) and 162550000 (162.550MHz), but otherwise the same __JSON Schema__ requirements apply.

#### JSON Schema
The channel data JSON file must contain a single unnamed array object that in turn contains one or more unnamed objects that describe each channel. Each unnamed channel object may include the following elements:
   
| Element | Type |Required | Description | Default |
| :-- | :-- | :-- | :-- | :--: |
| frequency | Integer  | __Yes__ | Specifies the channel frequency in Hertz. | __`N/A`__ |
| subchannel | Integer | No | Reserved for future use - must be set to __`0`__. | __`0`__ |
| modulation | String | No | Specifies the channel modulation. Set to __`"FM"`__ / __`"FMRADIO"`__ for analog Wideband FM, __`"HD"`__ / __`"HDRADIO"`__ for Hybrid Digital (HD) Radio, or __`"WX"`__ / __`"WEATHER"`__ for Weather Radio. | __`"FM"`__
| hidden | Integer | No | Specifies that the channel should be reported as hidden to Kodi. Set to __`1`__ to report the channel as hidden, __`0`__ to report the channel as visible.| __`0`__ |
| name | String | No | Specifies the name of the channel to report to Kodi. | __`""`__ |
| autogain | Integer | No | Specifies that tuner automatic gain control (AGC) should be used for this channel. Set to __`1`__ to enable AGC, __`0`__ to disable AGC. | __`0`__ |
| manualgain | Integer | No | Specifies that a manual antenna gain should used for this channel. Specified in tenths of a decibel. For example, to set manual gain of 32.8dB set to __`328`__. Ignored if autogain is set to __`1`__. <sup>1</sup> | __`0`__ |
| freqcorrection | Integer | No | Specifies a channel-specific frequency correction calibration offset (PPM) to apply in additional to the globally set value in the PVR __Device__ settings. | __`0`__
| logourl | String | No | Specifies the URL to a logo image to report to Kodi for display with this channel. | __`null`__ |

> <sup>1</sup> Valid manual gain values differ among RTL-SDR tuner devices. The PVR will automatically adjust what is specified here to the nearest valid value for the detected device. See below for a table that indicates the valid manual gain values for common RTL-SDR tuner devices.   
***
#### Example JSON
This example JSON describes six channels from the Baltimore, Maryland (US) metro using various options. WDCH-FM 99.1; WLIF-FM 101.9; WLIF-HD 101.9, WQSR-FM 102.7, WQSR-HD 102.7, and KEC83 (NOAA Weather Radio, 162.400MHz, WX2). WLIF and WQSR are imported twice, once each as an analog Wideband FM channel and again as a Hybrid Digital (HD) Radio channel:
```
[
    {
        "frequency": 99100000,
        "name": "WDCH-FM",
        "logourl": "https://upload.wikimedia.org/wikipedia/en/2/2b/WDCH_Bloomberg99.1-105.7HD2_logo.png"
    },
    {
        "frequency": 101900000,
        "modulation": "FM",
        "name": "WLIF-FM",
        "manualgain": 328,
        "logourl": "https://upload.wikimedia.org/wikipedia/en/4/4f/WLIF_logo_2013-.png"
    },
    {
        "frequency": 101900000,
        "modulation": "HD",
        "name": "WLIF-HD",
        "manualgain": 297,
        "freqcorrection": 8,
        "logourl": "https://upload.wikimedia.org/wikipedia/en/4/4f/WLIF_logo_2013-.png"
    },
    {
        "frequency": 102700000,
        "modulation": "FM",
        "name": "WQSR-FM",
        "autogain": 1,
        "logourl": null
    },
    {
        "frequency": 102700000,
        "modulation": "HD",
        "name": "WQSR-HD",
        "manualgain": 229,
        "freqcorrection": 6,
        "logourl": "https://upload.wikimedia.org/wikipedia/commons/4/40/Jackfmbaltimore.jpg"
    },
    {
        "frequency": 162400000,
        "modulation": "WX",
        "name": "KEC83",
        "autogain": 0,
        "manualgain": 480,
        "logourl": "https://upload.wikimedia.org/wikipedia/commons/thumb/6/6a/Noaa_all_hazards.svg/500px-Noaa_all_hazards.svg.png"
    }
]
```
***
#### RTL-SDR Tuner Device Manual Gain Values
| Device | Valid manual gains (tenths of a decibel) |
| :-- | :-- |
| Elonics E4000 | -10, 15, 40, 65, 90, 115, 140, 165, 190, 215, 240, 290, 340, 420 |
| Fitipower FC0012 | -99, -40, 71, 179, 192 |
| Fitipower FC0013 | -99, -73, -65, -63, -60, -58, -54, 58, 61, 63, 65, 67, 68, 70, 71, 179, 181, 182, 184, 186, 188, 191, 197 |
| FCI FC2580 | None |
| Rafael Micro R82xx | 0, 9, 14, 27, 37, 77, 87, 125, 144, 157, 166, 197, 207, 229, 254, 280, 297, 328, 338, 364, 372, 386, 402, 421, 434, 439, 445, 480, 496 |

