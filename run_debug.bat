@set SCRIPT_PATH=%~dp0
@pushd %SCRIPT_PATH%
@set RUN_ARGS=%*

@set APP_NAME=%1
@shift
@set RUN_ARGS=%1
:loop
@shift
@if not "%1"=="" (
    @set RUN_ARGS=%RUN_ARGS% %1
    @goto loop
)

@echo "Cleaning old dlls..."
del /F/Q %SCRIPT_PATH%\temp\game_dlls\*


call build_debug.bat %APP_NAME% || goto :build_error

@echo RUN_ARGS: %RUN_ARGS%
@if exist "%APP_NAME%.rdbg" (
    @set RUN_CMD=remedybg.exe -q -g %APP_NAME%.rdbg %RUN_ARGS%
) else (
	@echo %APP_NAME%.rdbg not found!
    @set RUN_CMD=remedybg.exe -q -g bazel-bin\kandinsky\main.exe --shared_lib bazel-bin\apps\%APP_NAME%\%APP_NAME%_shared.dll %RUN_ARGS%
)

@echo Running: %RUN_CMD%
%RUN_CMD% || goto :run_error

@goto :done

:build_error
:: We already logged an error message.
@goto :done

:run_error
@echo "RUN ERROR!"
exit /b 1

:done



