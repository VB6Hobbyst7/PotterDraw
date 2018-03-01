@echo off
copy C:\Chris\MyProjects\PotterDraw\web\*.* .
C:\Chris\MyProjects\tbl2web\release\tbl2web "..\PotterDraw ToDo.txt" issues.html issues.txt "PotterDraw Issues"
if errorlevel 1 goto err
C:\Chris\tools\navgen templateGitHub.html .
if errorlevel 1 goto err
C:\Chris\MyProjects\FixSelfUrl\Release\FixSelfUrl *.html
if errorlevel 1 goto err
md Help
del /s Help\*.htm
C:\Chris\MyProjects\doc2web\release\doc2web /nospaces C:\Chris\MyProjects\PotterDraw\Help\help.txt Help Contents.htm C:\Chris\MyProjects\PotterDraw\info\PotterDrawHelp.htm "PotterDraw Help"
if errorlevel 1 goto err
cd Help
md images
del /y images\*.*
copy C:\Chris\MyProjects\PotterDraw\Help\images\*.* images
if errorlevel 1 goto err
copy ..\helptopic.css content.css
C:\Chris\tools\navgen C:\Chris\MyProjects\PotterDraw\Help\template.txt .
copy ..\helpheader.txt x
copy x + Contents.htm
echo ^<body^>^<html^> >>x
del Contents.htm
ren x Contents.htm
md printable
cd printable
move C:\Chris\MyProjects\PotterDraw\info\PotterDrawHelp.htm .
ren PotterDrawHelp.htm prnhelp.htm
echo y | C:\Chris\tools\fsr prnhelp.htm "../../../images/" "https://potterdraw.sourceforge.io/Help/images/"
echo y | C:\Chris\tools\fsr prnhelp.htm "../../images/" "https://potterdraw.sourceforge.io/Help/images/"
echo y | C:\Chris\tools\fsr prnhelp.htm "../images/" "https://potterdraw.sourceforge.io/Help/images/"
ren prnhelp.htm PotterDrawHelp.htm
cd ..
cd ..
ren issues.html issues.htm
echo y | fsr issues.htm "<div id=body>" "<div id=widebody>"
ren issues.htm issues.html
goto exit
:err
pause Error!
:exit
