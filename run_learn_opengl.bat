@set SCRIPT_PATH=%~dp0
@pushd %SCRIPT_PATH%
@set ARGS=%*

call build_debug.bat %ARGS% || goto :build_error

::bazel-bin\kandinsky\main.exe || goto :run_error
REM if exist "kandinsy.rdbg" (
REM     remedybg -q -g kandinsky.rdbg
REM ) else (
    remedybg -q -g bazel-bin\kandinsky\main.exe bazel-bin\apps\learn_opengl\learn_opengl_shared.dll
REM )

@goto :done

:build_error
:: We already logged an error message.
@goto :done

:run_error
@echo "RUN ERROR!"
exit /b 1

:done



