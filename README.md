# __pvr.rtlradio__  

Realtek RTL2832U RTL-SDR FM Radio PVR Client   
   
Copyright (C)2020 Michael G. Brehm    
[MIT LICENSE](https://opensource.org/licenses/MIT)   
   
Concept based on [__pvr.rtl.radiofm__](https://github.com/AlwinEsch/pvr.rtl.radiofm) - Copyright (C) 2015-2018 Alwin Esch   
Wideband FM Digital Signal Processing provided by [__CuteSDR__](https://sourceforge.net/projects/cutesdr/) - Copyright (C) 2010 Moe Wheatley   
   
## BUILD ENVIRONMENT
**REQUIRED COMPONENTS**   
* Windows 10 x64 1909 (18363) "November 2019 Update"   
* Visual Studio 2017 (Windows 10 SDK (10.0.17763.0)   
* Windows Subsystem for Linux   
* [Ubuntu on Windows 16.04 LTS](https://www.microsoft.com/store/productId/9PJN388HP8C9)   

**OPTIONAL COMPONENTS**   
* Android NDK r20b for Windows 64-bit   
   
**REQUIRED: CONFIGURE UBUNTU ON WINDOWS**   
* Open "Ubuntu"   
```
sudo dpkg --add-architecture i386
sudo apt-get update
sudo apt-get install gcc-4.9 g++-4.9 libc6-dev:i386 libstdc++-4.9-dev:i386 lib32gcc-4.9-dev 
sudo apt-get install gcc-4.9-arm-linux-gnueabihf g++-4.9-arm-linux-gnueabihf gcc-4.9-arm-linux-gnueabi g++-4.9-arm-linux-gnueabi gcc-4.9-aarch64-linux-gnu g++-4.9-aarch64-linux-gnu
```
   
**OPTIONAL: CONFIGURE ANDROID NDK**   
*Necessary to build Android Targets*   
   
Download the Android NDK r20b for Windows 64-bit:    
[https://dl.google.com/android/repository/android-ndk-r20b-windows-x86_64.zip](https://dl.google.com/android/repository/android-ndk-r20b-windows-x86_64.zip)   

* Extract the contents of the .zip file somewhere   
* Set a System Environment Variable named ANDROID_NDK_ROOT that points to the extracted android-ndk-r20b folder
   
## BUILD KODI ADDON PACKAGES
**INITIALIZE SOURCE TREE AND DEPENDENCIES**
* Open "Developer Command Prompt for VS2017"   
```
git clone https://github.com/djp952/pvr.rtlradio -b Matrix
cd pvr.rtlradio
git submodule update --init
```
   
**BUILD ADDON TARGET PACKAGE(S)**   
* Open "Developer Command Prompt for VS2017"   
```
cd pvr.rtlradio
msbuild msbuild.proj [/t:target[;target...]] [/p:parameter=value[;parameter=value...]
```
   
**SUPPORTED TARGET PLATFORMS**   
The MSBUILD script can be used to target one or more platforms by specifying the /t:target parameter.  The tables below list the available target platforms and the required command line argument(s) to pass to MSBUILD.  In the absence of the /t:target parameter, the default target selection is **windows**.
   
**SETTING DISPLAY VERSION**   
The display version of the addon that will appear in the addon.xml and to the user in Kodi may be overridden by specifying a /p:DisplayVerison={version} argument to any of the build targets.  By default, the display version will be the first three components of the full build number \[MAJOR.MINOR.REVISION\].  For example, to force the display version to the value '2.0.3a', specify /p:DisplayVersion=2.0.3a on the MSBUILD command line.
   
Examples:   
   
> Build just the linux-armhf platform:   
> ```
>msbuild /t:linux-armhf
> ```
   
> Build all Linux platforms, force display version to '2.0.3a':   
> ```
> msbuild /t:linux /p:DisplayVersion=2.0.3a
> ```
   
*INDIVIDUAL TARGETS*    
   
| Target | Platform | MSBUILD Parameters |
| :-- | :-- | :-- |
| android-aarch64 | Android ARM64 | /t:android-aarch64 |
| android-arm | Android ARM | /t:android-arm |
| android-x86 | Android X86 | /t:android-x86 |
| linux-aarch64 | Linux ARM64 | /t:linux-aarch64 |
| linux-armel | Linux ARM | /t:linux-armel |
| linux-armhf | Linux ARM (hard float) | /t:linux-armhf |
| linux-i686 | Linux X86 | /t:linux-i686 |
| linux-x86\_64 | Linux X64 | /t:linux-x86\_64 |
| windows-win32 | Windows X86 | /t:windows-win32 |
| windows-x64 | Windows X64 | /t:windows-x64 |
   
*COMBINATION TARGETS*   
   
| Target | Platform(s) | MSBUILD Parameters |
| :-- | :-- | :-- |
| all | All targets | /t:all |
| android | All Android targets | /t:android |
| linux | All Linux targets | /t:linux |
| windows (default) | All Windows targets | /t:windows |
   
