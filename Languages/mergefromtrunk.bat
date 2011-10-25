@echo off
setlocal
SET TRUNK=..\..\..\trunk\Languages

REM Merge translations from trunk and branch into the branch translation
REM msgcat deliberately uses trunk first to overwrite old translations 
REM with newer ones from trunk!
REM If we used branch first, we'd get only missing translations from trunk

FOR /D %%L IN (*.) DO (
  echo Merging %%L
  IF EXIST %%L\TortoiseUI.po (
    echo - GUI Translation
    msgcat --use-first %TRUNK%\%%L\TortoiseUI.po %%L\TortoiseUI.po -o temp.po 2>nul
    msgmerge temp.po TortoiseUI.pot -o %%L\TortoiseUI.po 2>nul
  )
  IF EXIST %%L\TortoiseDoc.po (
    echo - Doc Translation
    msgcat --use-first %TRUNK%\%%L\TortoiseDoc.po %%L\TortoiseDoc.po -o temp.po
    msgmerge temp.po TortoiseDoc.pot -o %%L\TortoiseDoc.po
  )
)

del temp.po
del messages.mo

endlocal
