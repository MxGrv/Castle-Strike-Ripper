FOR /R "%CD%\Output" %%I IN (*.zipped.*) DO (
 offzip "%%I" "%%~dI%%~pI%%~nI.unzipped%%~xI"
)
