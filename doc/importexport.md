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
***
#### JSON Schema
The channel data JSON file must contain a single unnamed array object that in turn contains one or more unnamed objects that describe each channel. Each unnamed channel object may include the following elements:
   
| Element | Type|Required | Description | Default |
| :-- | :-- | :-- | :-- | :--: |
| frequency | Integer  | Yes | Specifies the channel frequency in Hertz. | __`N/A`__ |
| subchannel | Integer | Yes | Reserved for future use - must be set to __`0`__. | __`N/A`__ |
| hidden | Integer | No | Specifies that the channel should be reported as hidden to Kodi.  Set to __`1`__ to report the channel as hidden, __`0`__ to report the channel as visible.| __`0`__ |
| name | String | No | Specifies the name of the channel to report to Kodi. | __`""`__ |
| autogain | Integer | No | Specifies that tuner automatic gain control (AGC) should be used for this channel. Set to __`1`__ to enable AGC, __`0`__ to disable AGC. | __`1`__ |
| manualgain | Integer | No | Specifies that a manual antenna gain should used for this channel. Specified in tenths of a decibel. For example, to set manual gain of 32.8dB set to __`328`__. Ignored if autogain is set to __`1`__. <sup>1</sup> | __`0`__ |
| logourl | String | No | Specifies the URL to a logo image to report to Kodi for display with this channel. | __`null`__ |

> <sup>1</sup> Valid manual gain values differ among RTL-SDR tuner devices. The PVR will automatically adjust what is specified here to the nearest valid value for the detected device. See below for a table that indicates the valid manual gain values for common RTL-SDR tuner devices.   
***
#### Example JSON:
```
[
    {
        "frequency": 99100000,
        "subchannel": 0,
        "hidden": 0,
        "name": "WDCH-FM",
        "autogain": 1,
        "manualgain": 0,
        "logourl": "https://notreal.org/WDCH_Bloomberg99.1-105.7HD2_logo.png"
    },
    {
        "frequency": 101900000,
        "subchannel": 0,
        "hidden": 0,
        "name": "WLIF-FM",
        "autogain": 0,
        "manualgain": 328,
        "logourl": "https://notreal.org/220px-WLIF_logo_2013-.png"
    },
    {
        "frequency": 102700000,
        "subchannel": 0,
        "hidden": 0,
        "name": "WQSR-FM",
        "autogain": 0,
        "manualgain": 480,
        "logourl": null
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

