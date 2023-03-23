@ECHO off

ECHO ---------------------------------------------------------------------------
ECHO MSVC compiling [%date% %time%]
ECHO ---------------------------------------------------------------------------

SET PDB_NAME_DATE="engine_%random%.pdb"
SET MAINFILE="../../src/win32_main.cpp"
SET SDL_VER="SDL2-2.0.12"

SET INCLUDE_HEADERS= -I "../../libs/%SDL_VER%/include"
SET LIBS=shell32.lib
SET INCLUDE_LIBS="../../libs/%SDL_VER%/lib/x64/SDL2.lib" "../../libs/%SDL_VER%/lib/x64/SDL2main.lib" %LIBS%
REM Add the flag /DGC_DEBUG_MODE=1 if you want to enable the debug mode.
SET CONSTANTS=/DENGINE_TEST_MODE=0 /DENGINE_HOTLOAD=1 /DSDL_ASSERT_LEVEL=2
SET WARNINGS=-wd4996 -wd4100 -wd4201 -wd4700 -wd4324 -wd4244 -wd4311 -wd4312 -wd4302 -wd4706 -wd4334 -wd4132 -wd4189
SET COMMON_COMPILER_FLAGS=-O2 -MTd -nologo -fp:fast -fp:except- -GR- -EHa- -Zo -Oi -WX -W4 -FC -Z7 /EHsc %WARNINGS% %INCLUDE_HEADERS%
SET COMMON_LINKER_FLAGS=/subsystem:console /incremental:no /opt:ref
SET LINKER_DLL_FLAGS=-incremental:no -opt:ref -PDB:engine_%random%.pdb -EXPORT:get_engine_api /DEBUG:FULL
SET C_STANDARD=/Tc

MKDIR build\msvc
PUSHD build\msvc
DEL *.pdb > NUL 2> NUL

ECHO WAITING FOR PDB > lock.tmp
cl %CONSTANTS% %COMMON_COMPILER_FLAGS% %INCLUDE_LIBS% %C_STANDARD% ..\..\src\gcsr_engine_core.cpp -LD /link /MAP:gcsr_engine_core.map %LINKER_DLL_FLAGS%
DEL lock.tmp

ECHO ---------------------------------------------------------------------------
cl %CONSTANTS% %COMMON_COMPILER_FLAGS% %INCLUDE_LIBS% -Fe:"software_renderer" %C_STANDARD% %MAINFILE% /link /MAP:win32_main.map %COMMON_LINKER_FLAGS%
ECHO ---------------------------------------------------------------------------

cl %CONSTANTS% %COMMON_COMPILER_FLAGS% -Fe:"asset_builder" %C_STANDARD% ..\..\src\asset_builder\gcsr_asset_builder.cpp %INCLUDE_LIBS% /link /MAP:gcsr_asset_builder.map %COMMON_LINKER_FLAGS%

POPD build\msvc

ECHO ---------------------------------------------------------------------------
ECHO Copying to bin ...
ECHO ---------------------------------------------------------------------------

DEL /Q bin\msvc
MKDIR bin\msvc

CP build\msvc\software_renderer.exe bin\msvc
CP build\msvc\asset_builder.exe bin\msvc
CP build\msvc\gcsr_engine_core.dll bin\msvc

ECHO ---------------------------------------------------------------------------
ECHO Finished [%date% %time%]
ECHO ---------------------------------------------------------------------------
