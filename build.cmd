@echo off
cd %~dp0
bash -c "make" || exit

::set path=C:\SDK\STM32Tools\tools\win;%path%
::call maple_upload.bat COM5 2 1EAF:0003 %file%

set file=%~dp0\build\main.hex
set drive=e:
copy %file% %drive%\



