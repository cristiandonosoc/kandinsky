@set SCRIPT_PATH=%~dp0
@pushd %SCRIPT_PATH%
@set ARGS=%*

call build_debug.bat %ARGS% || goto :build_error

::bazel-bin\kandinsky\main.exe || goto :run_error
@if exist "learn_opengl.rdbg" (
    remedybg.exe -q -g learn_opengl.rdbg
) else (
	@echo learn_opengl.rdbg not found!
    remedybg.exe -q -g bazel-bin\kandinsky\main.exe --shared_lib bazel-bin\apps\learn_opengl\learn_opengl_shared.dll
)

@goto :done

:build_error
:: We already logged an error message.
@goto :done

:run_error
@echo "RUN ERROR!"
exit /b 1

:done



