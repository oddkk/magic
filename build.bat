@echo off

mkdir build
pushd build

set SRC=
set FLAGS=

for /R ..\src\ %%f in (*.c) do call set SRC=%%SRC%% "%%f"

for /R ..\vendor\mathc\ %%f in (*.c) do call set SRC=%%SRC%% "%%f"
set FLAGS=%FLAGS% /I ..\vendor\mathc\

for /R ..\vendor\glad\src\ %%f in (*.c) do call set SRC=%%SRC%% "%%f"
set FLAGS=%FLAGS% /I ..\vendor\glad\include\

set SRC=%SRC% "..\vendor\tracy\TracyClient.cpp"
set FLAGS=%FLAGS% /I ..\vendor\tracy\ -DTRACY_ENABLE

:: set SRC=%SRC% "..\vendor\glfw\src\context.c" "..\vendor\glfw\patches\init.c" "..\vendor\glfw\patches\input.c" "..\vendor\glfw\patches\monitor.c" "..\vendor\glfw\src\vulkan.c" "..\vendor\glfw\patches\window.c" "..\vendor\glfw\src\win32_init.c" "..\vendor\glfw\src\win32_joystick.c" "..\vendor\glfw\src\win32_monitor.c" "..\vendor\glfw\src\win32_thread.c" "..\vendor\glfw\src\win32_time.c" "..\vendor\glfw\src\win32_window.c" "..\vendor\glfw\src\wgl_context.c"
set SRC=%SRC% "..\vendor\glfw\src\context.c" "..\vendor\glfw\src\init.c" "..\vendor\glfw\src\input.c" "..\vendor\glfw\src\monitor.c" "..\vendor\glfw\src\vulkan.c" "..\vendor\glfw\src\window.c" "..\vendor\glfw\src\win32_init.c" "..\vendor\glfw\src\win32_joystick.c" "..\vendor\glfw\src\win32_monitor.c" "..\vendor\glfw\src\win32_thread.c" "..\vendor\glfw\src\win32_time.c" "..\vendor\glfw\src\win32_window.c" "..\vendor\glfw\src\wgl_context.c" "..\vendor\glfw\src\egl_context.c" "..\vendor\glfw\src\osmesa_context.c"
set FLAGS=%FLAGS% /I ..\vendor\glfw\include\ /D_GLFW_WIN32=1 gdi32.lib user32.lib kernel32.lib shell32.lib shlwapi.lib

mkdir .\obj\
cl /O2 /Zi /Fe"..\magic.exe" %FLAGS% %SRC%

popd
