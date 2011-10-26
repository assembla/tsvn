@echo off

rem Merge translations from trunk and branch into the branch translation
rem Needs the following tools installed in your search path:
rem - Gnu gettext to combine and merge the .po files
rem - dos2unix to make sure that the line endings are consistent

setlocal
set TRUNK=..\..\..\trunk\Languages

rem msgcat deliberately uses trunk first to overwrite old translations 
rem with newer ones from trunk!
rem If we used branch first, we'd get only missing translations from trunk

for /D %%L in (*.) DO (
  echo Merging %%L
  if exist %%L\TortoiseUI.po (
    echo - GUI Translation
    msgcat --use-first %TRUNK%\%%L\TortoiseUI.po %%L\TortoiseUI.po -o temp.po 2>nul
    msgmerge temp.po TortoiseUI.pot -o %%L\TortoiseUI.po 2>nul
	dos2unix %%L\TortoiseUI.po
  )
  if exist %%L\TortoiseDoc.po (
    echo - Doc Translation
    msgcat --use-first %TRUNK%\%%L\TortoiseDoc.po %%L\TortoiseDoc.po -o temp.po
    msgmerge temp.po TortoiseDoc.pot -o %%L\TortoiseDoc.po
	dos2unix %%L\TortoiseDoc.po
  )
)

del temp.po
del messages.mo

endlocal
