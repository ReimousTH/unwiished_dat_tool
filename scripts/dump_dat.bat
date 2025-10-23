@set "bat_dir=%~dp0"
@set "dat_file=%1"
@set "tool=unwiished_dat_tool.exe"
@cd "%bat_dir%"
@%tool% %dat_file% --read
@PAUSE