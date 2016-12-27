@echo off
cl.exe /EHsc /Ox /GS- /guard:cf- /GL /W4 /analyze /Zi all.cpp -FePlaceholderProjectReleaseUni.exe
