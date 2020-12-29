cd "%~dp0"
call "c:\Program Files\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars32.bat"
cl.exe /std:c++17 /EHsc pcap_player.cpp /link ws2_32.lib
