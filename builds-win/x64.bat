@REM build a 32-bit version of the library

set curdir=%cd%
set root=%cd%
call C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat
cd %root%
echo Going to %root%
set vcpkgdir=C:\vcpkg\installed\x64-windows

cl.exe /dll /MT /D_USRDLL /D_WINDLL /DWIN32 /D_WIN32 /I .\nanopb /I %root%\secp256k1-vrf\include  /I %vcpkgdir%\include  /DMBEDTLS_PLATFORM_C  libcurl.lib secp256k1-vrf.lib ws2_32.lib  aergo.c  win32/dllmain.c /LD /Felibaergo-1.1.dll /link /LIBPATH:%root%\x64 /LIBPATH:%vcpkgdir%\lib

@REM Move 64-bit dlls

set outdir=builds-win\x64
mkdir %outdir%
move libaergo-1.1.dll %outdir%\
move libaergo-1.1.lib %outdir%\
copy %vcpkgdir%\bin\libcurl.dll %outdir%\
copy %vcpkgdir%\bin\zlib1.dll %outdir%\
copy %vcpkgdir%\bin\nghttp2.dll %outdir%\

copy %root%\x64\secp256k1-vrf.dll %outdir%\

@REM Compile test program 1

cl.exe /I%root% examples\account_info\account_info.c %outdir%\libaergo-1.1.lib

move account_info.exe %outdir%
