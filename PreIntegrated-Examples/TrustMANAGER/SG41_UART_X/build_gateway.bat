@echo off
REM ============================================================================
REM KTA Gateway - Windows Build Script (MinGW GCC)
REM ============================================================================
REM Usage: build_gateway.bat [clean]
REM Output: gateway\kta_gateway.exe
REM ============================================================================

setlocal

set GW=gateway
set OUT=%GW%\kta_gateway.exe
set CC=gcc

REM Handle "clean" argument
if "%1"=="clean" (
    if exist %OUT% del %OUT%
    echo Cleaned.
    goto :eof
)

REM ============================================================================
REM Include paths
REM ============================================================================
set INCS=-I.
set INCS=%INCS% -I%GW%
set INCS=%INCS% -I%GW%\ktaIntegration
set INCS=%INCS% -I%GW%\ktaIntegration\platform\include
set INCS=%INCS% -I%GW%\backends
set INCS=%INCS% -I%GW%\backends\uart
set INCS=%INCS% -I%GW%\backends\uart\sal\windows
set INCS=%INCS% -I%GW%\keyStreamIntegration\COMMSTACK\http\include
REM KTA log module + its config/SAL headers (LOG_KTA_ENABLE comes from ktaConfig.h)
set INCS=%INCS% -I%GW%\keyStreamIntegration\COMMON\logmodule\include
set INCS=%INCS% -Ikeystream_connect\src\config\default\library\kta_lib\SOURCE\include
set INCS=%INCS% -Ikeystream_connect\src\config\default\library\kta_lib\SOURCE\salapi\include

REM ============================================================================
REM Source files
REM ============================================================================
set SRCS=
set SRCS=%SRCS% %GW%\application\main_windows_async_kta.c
set SRCS=%SRCS% %GW%\ktaIntegration\ktaFieldMgntHook.c
set SRCS=%SRCS% %GW%\ktaIntegration\platform\windows\kta_async_client.c
set SRCS=%SRCS% %GW%\backends\backend_interface.c
set SRCS=%SRCS% %GW%\backends\backend_message.c
set SRCS=%SRCS% %GW%\backends\uart\backend_uart.c
set SRCS=%SRCS% %GW%\backends\uart\sal\windows\uart_sal.c
set SRCS=%SRCS% %GW%\keyStreamIntegration\COMMSTACK\http\comm_if.c
set SRCS=%SRCS% %GW%\keyStreamIntegration\COMMSTACK\http\http.c
set SRCS=%SRCS% %GW%\keyStreamIntegration\COMMSTACK\http\windows\k_sal_com.c
set SRCS=%SRCS% %GW%\keyStreamIntegration\COMMON\logmodule\KTALog.c
set SRCS=%SRCS% %GW%\keyStreamIntegration\COMMON\logmodule\k_sal_log_win.c

REM ============================================================================
REM Compiler flags
REM ============================================================================
set CFLAGS=-Wall -Wextra -O2
set CFLAGS=%CFLAGS% -D_WIN32 -DWIN32_LEAN_AND_MEAN
set CFLAGS=%CFLAGS% -DKTA_CLIENT_BACKEND=BACKEND_TYPE_UART
set CFLAGS=%CFLAGS% -DKTA_ENABLE_LOGGING
REM Enable KTA log module output (gateway only). The Windows log backend
REM (k_sal_log_win.c) writes to stdout, so this is safe here even though the
REM shared ktaConfig.h keeps logging off for the MCU build (USB CDC blocks).
set CFLAGS=%CFLAGS% -DLOG_KTA_ENABLE=C_KTA_LOG_LEVEL_DEBUG

REM Libraries
set LIBS=-lws2_32

echo Building KTA Gateway...
echo.

%CC% %CFLAGS% %INCS% %SRCS% -o %OUT% %LIBS%

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Build successful: %OUT%
) else (
    echo.
    echo BUILD FAILED
    exit /b 1
)
