@echo off
setlocal enabledelayedexpansion

set "bat_dir=%~dp0"
set "dat_file=%~1"
set "dat_dir=%~dp1"
set "tool=unwiished_dat_tool.exe"

cd /d "%bat_dir%"

set "all_padding="
for /f "usebackq tokens=*" %%A in ("padding_entries.source") do (
    set "line=%%A"
    if not "!line!"=="" if not "!line:~0,1!"==";" (
        set "padding_entry=!line:dat_dir#=!"
        set "all_padding=!all_padding!%dat_dir%!padding_entry! "
    )
)

echo Command being executed:
echo "%tool%" "%dat_file%" --build --local_padding=!all_padding!
"%tool%" "%dat_file%" --build --local_padding="!all_padding!"

pause