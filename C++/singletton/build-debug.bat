@echo off
cl.exe /EHsc /Od /GS- /Oy- /guard:cf- /GL /W4 /analyze /Zi /D DEBUG main.cpp main2.cpp singletton1.cpp singletton2.cpp singletton3.cpp singletton4.cpp -FePlaceholderProjectDebug.exe
