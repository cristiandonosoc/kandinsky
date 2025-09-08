@set SCRIPT_PATH=%~dp0
@pushd %SCRIPT_PATH%
@set ARGS=%*

@set APP_NAME=%1
@shift
@set ARGS=%1
:loop
@shift
@if not "%1"=="" (
    @set ARGS=%ARGS% %1
    @goto loop
)

call build_debug.bat %APP_NAME% %ARGS% || goto :build_error

::bazel-bin\kandinsky\main.exe || goto :run_error
@if exist "%APP_NAME%.rdbg" (
    remedybg.exe -q -g %APP_NAME%.rdbg
) else (
	@echo %APP_NAME%.rdbg not found!
    remedybg.exe -q -g bazel-bin\kandinsky\main.exe --shared_lib bazel-bin\apps\%APP_NAME%\%APP_NAME%_shared.dll
)

@goto :done

:build_error
:: We already logged an error message.
@goto :done

:run_error
@echo "RUN ERROR!"
exit /b 1

:done



