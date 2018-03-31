set CURR_DIR=%cd%

if not defined DevEnvDir (
    call "%VS_CMD_LINE_BUILD_PATH%" x64
)

cd /d %CURR_DIR%

set SOURCES=../main.c

pushd bin
cl %SOURCES% /I.. /EHsc /Zi /MP8 /Fesprite_extractor.exe 
popd
