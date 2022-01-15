# __pvr.rtlradio__  

Realtek RTL2832U RTL-SDR FM Radio PVR Client   
   
Copyright (C)2020-2022 Michael G. Brehm    
[MIT LICENSE](https://opensource.org/licenses/MIT)   
   
Concept based on [__pvr.rtl.radiofm__](https://github.com/AlwinEsch/pvr.rtl.radiofm) - Copyright (C) 2015-2018 Alwin Esch   
Wideband FM Digital Signal Processing provided by [__CuteSDR__](https://sourceforge.net/projects/cutesdr/) - Copyright (C) 2010 Moe Wheatley   
   
[__LIBUSB__](https://libusb.info/) - Copyright (C)2012-2020 libusb   
[__RAPIDJSON__](https://rapidjson.org/) - Copyright (C)2015 THL A29 Limited, a Tencent company, and Milo Yip   
[__RTL-SDR__](https://osmocom.org/projects/rtl-sdr/wiki/Rtl-sdr/) - Copyright (C)2012-2013 Steve Markgraf, Dimitri Stolnikov   
   
## BUILD ENVIRONMENT
**REQUIRED COMPONENTS**   
* Windows 10 x64 20H2 (19042) or later   
* Visual Studio 2022 Community Edition or higher with:    
     * Desktop Development with C++   
     * Windows 10 SDK (10.0.18362.0)
     * MVSC v141 VS2017 C++ x64/x86 build tools (v14.16)
* [Windows Subsystem for Linux](https://docs.microsoft.com/en-us/windows/wsl/install-win10) (WSL v1 recommended)   
* [WSL Ubuntu 18.04 LTS Distro](https://www.microsoft.com/store/productId/9N9TNGVNDL3Q)   

**OPTIONAL COMPONENTS**   
* Android NDK r20b for Windows 64-bit   
* OSXCROSS Cross-Compiler (with Mac OSX 10.11 SDK)   
   
**REQUIRED: CONFIGURE UBUNTU ON WINDOWS**   
* Open "Ubuntu 18.04 LTS"   
```
sudo dpkg --add-architecture i386
sudo add-apt-repository 'deb http://archive.ubuntu.com/ubuntu/ xenial main universe'
sudo apt-get update
sudo apt-get install gcc-4.9 g++-4.9 libc6-dev:i386 libstdc++-4.9-dev:i386 lib32gcc-4.9-dev
sudo apt-get install gcc-4.9-arm-linux-gnueabihf g++-4.9-arm-linux-gnueabihf gcc-4.9-arm-linux-gnueabi g++-4.9-arm-linux-gnueabi gcc-4.9-aarch64-linux-gnu g++-4.9-aarch64-linux-gnu
sudo apt-get install libgles2-mesa-dev libgles2-mesa-dev:i386
```
   
**OPTIONAL: CONFIGURE ANDROID NDK**   
*Necessary to build Android Targets*   
   
Download the Android NDK r20b for Windows 64-bit:    
[https://dl.google.com/android/repository/android-ndk-r20b-windows-x86_64.zip](https://dl.google.com/android/repository/android-ndk-r20b-windows-x86_64.zip)   

* Extract the contents of the .zip file somewhere   
* Set a System Environment Variable named ANDROID_NDK_ROOT that points to the extracted android-ndk-r20b folder
   
**OPTIONAL: CONFIGURE OSXCROSS CROSS-COMPILER**   
*Necessary to build OS X Targets*   

* Generate the MAC OSX 10.11 SDK Package for OSXCROSS by following the instructions provided at [PACKAGING THE SDK](https://github.com/tpoechtrager/osxcross#packaging-the-sdk).  The suggested version of Xcode to use when generating the SDK package is Xcode 7.3.1 (May 3, 2016).
* Open "Ubuntu 18.04 LTS"   
```
sudo apt-get install cmake clang llvm-dev libxml2-dev libssl-dev libbz2-dev zlib1g-dev
git clone https://github.com/tpoechtrager/osxcross --depth=1
cp {MacOSX10.11.sdk.tar.bz2} osxcross/tarballs/
UNATTENDED=1 osxcross/build.sh
osxcross/build_compiler_rt.sh
sudo mkdir -p /usr/lib/llvm-6.0/lib/clang/6.0.0/include
sudo mkdir -p /usr/lib/llvm-6.0/lib/clang/6.0.0/lib/darwin
sudo cp -rv $(pwd)/osxcross/build/compiler-rt/compiler-rt/include/sanitizer /usr/lib/llvm-6.0/lib/clang/6.0.0/include
sudo cp -v $(pwd)/osxcross/build/compiler-rt/compiler-rt/build/lib/darwin/*.a /usr/lib/llvm-6.0/lib/clang/6.0.0/lib/darwin
sudo cp -v $(pwd)/osxcross/build/compiler-rt/compiler-rt/build/lib/darwin/*.dylib /usr/lib/llvm-6.0/lib/clang/6.0.0/lib/darwin
```
   
## BUILD KODI ADDON PACKAGES
**INITIALIZE SOURCE TREE AND DEPENDENCIES**
* Open "Developer Command Prompt for VS2022"   
```
git clone https://github.com/djp952/pvr.rtlradio -b Nexus
cd pvr.rtlradio
git submodule update --init
```
   
**BUILD ADDON TARGET PACKAGE(S)**   
* Open "Developer Command Prompt for VS2022"   
```
cd pvr.rtlradio
msbuild msbuild.proj [/t:target[;target...]] [/p:parameter=value[;parameter=value...]
```
   
**SUPPORTED TARGET PLATFORMS**   
The MSBUILD script can be used to target one or more platforms by specifying the /t:target parameter.  The tables below list the available target platforms and the required command line argument(s) to pass to MSBUILD.  In the absence of the /t:target parameter, the default target selection is **windows**.
   
**SETTING DISPLAY VERSION**   
The display version of the addon that will appear in the addon.xml and to the user in Kodi may be overridden by specifying a /p:DisplayVerison={version} argument to any of the build targets.  By default, the display version will be the first three components of the full build number \[MAJOR.MINOR.REVISION\].  For example, to force the display version to the value '2.0.3a', specify /p:DisplayVersion=2.0.3a on the MSBUILD command line.
   
Examples:   
   
> Build just the osx-x86\_64 platform:   
> ```
>msbuild /t:osx-x86_64
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
| osx-x86\_64 | Mac OS X X64 | /t:osx-x86\_64 |
| windows-win32 | Windows X86 | /t:windows-win32 |
| windows-x64 | Windows X64 | /t:windows-x64 |
   
*COMBINATION TARGETS*   
   
| Target | Platform(s) | MSBUILD Parameters |
| :-- | :-- | :-- |
| all | All targets | /t:all |
| android | All Android targets | /t:android |
| linux | All Linux targets | /t:linux |
| osx | All Mac OS X targets | /t:osx |
| windows (default) | All Windows targets | /t:windows |
   
## ADDITIONAL LICENSE INFORMATION
   
**LIBUSB**   
[https://www.gnu.org/licenses/gpl-faq.html#LGPLStaticVsDynamic](https://www.gnu.org/licenses/gpl-faq.html#LGPLStaticVsDynamic)   
This library statically links with code licensed under the GNU Lesser Public License, v2.1 [(LGPLv2.1)](https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html).  As per the terms of that license, the maintainer (djp952) must provide the library in an object (or source) format and allow the user to modify and relink against a different version(s) of the LGPL 2.1 libraries.  To use a different or custom version of libusb the user may alter the contents of the depends/libusb source directory prior to building this library.   
   
**RTL-SDR**   
The RTL-SDR library is licensed under the GNU Public License, v2 [(GPLv2)](https://www.gnu.org/licenses/old-licenses/gpl-2.0.html). The overarching license model for this project is assumed to be compatible with GPLv2 given the [(commentary)](https://www.gnu.org/licenses/license-list.en.html#GPLCompatibleLicenses) on this matter. If this project is deemed to be incompatible or otherwise in violation of the GPLv2 license, please contact the maintainer (djp952) so that appropriate remedies can be applied.
   
**XCODE AND APPLE SDKS AGREEMENT**   
The instructions provided above indirectly reference the use of intellectual material that is the property of Apple, Inc.  This intellectual material is not FOSS (Free and Open Source Software) and by using it you agree to be bound by the terms and conditions set forth by Apple, Inc. in the [Xcode and Apple SDKs Agreement](https://www.apple.com/legal/sla/docs/xcode.pdf).
