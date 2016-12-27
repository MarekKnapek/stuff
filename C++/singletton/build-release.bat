@echo off
cl.exe /EHsc /Ox /GS- /guard:cf- /GL /W4 /analyze /Zi main.cpp main2.cpp singletton1.cpp singletton2.cpp singletton3.cpp singletton4.cpp -FePlaceholderProjectRelease.exe
