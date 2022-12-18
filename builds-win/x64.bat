@REM build a 64-bit version of the library

set outdir=builds-win\x64
set root=%cd%
call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
set vcpkgdir=C:\vcpkg\installed\x64-windows

cl.exe /dll /MT /D_USRDLL /D_WINDLL /DWIN32 /D_WIN32 /I .\nanopb /I %root%\secp256k1-vrf\include  /I %vcpkgdir%\include  /DMBEDTLS_PLATFORM_C  libcurl.lib secp256k1-vrf.lib ws2_32.lib  aergo.c  win32/dllmain.c /LD /Felibaergo-1.1.dll /link /LIBPATH:%root%\x64 /LIBPATH:%vcpkgdir%\lib

@REM Move 64-bit dll
mkdir %outdir%
copy libaergo-1.1.dll %outdir%\
copy libaergo-1.1.lib %outdir%\

if [not] %environment%=="ci" (
copy %vcpkgdir%\bin\libcurl.dll %outdir%\
copy %vcpkgdir%\bin\zlib1.dll %outdir%\
copy %vcpkgdir%\bin\nghttp2.dll %outdir%\
copy %root%\x64\secp256k1-vrf.dll %outdir%\
)
