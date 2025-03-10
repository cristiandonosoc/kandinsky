@set SCRIPT_PATH=%~dp0
@pushd %SCRIPT_PATH%
@set ARGS=%*

call build_debug.bat %ARGS% || goto :build_error

::bazel-bin\kandinsky\main.exe || goto :run_error
@if exist "learn_opengl.rdbg" (
    remedybg -q -g learn_opengl.rdbg
) else (
    @REM remedybg -q -g bazel-bin\kandinsky\main.exe -- bazel-bin\apps\learn_opengl\shared.dll
	@echo learn_opengl.rdbg not found!
	@goto :error
)



@goto :done

:build_error
:: We already logged an error message.
@goto :done

:run_error
@echo "RUN ERROR!"
exit /b 1

:done



