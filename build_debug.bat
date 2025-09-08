@set SCRIPT_PATH=%~dp0
@pushd %SCRIPT_PATH%

@if "%1"=="" (
    @echo Usage: %0 ^<app_name^> [additional_args...]
    @echo Example: %0 learn_opengl --config=debug
    @exit /b 1
)

@set APP_NAME=%1
@shift
@set ARGS=%1
:loop
@shift
@if not "%1"=="" (
    @set ARGS=%ARGS% %1
    @goto loop
)

@echo VALIDATING SHADERS ---------------------------------------------------------------------------
@call validate_shaders || goto :build_error

@echo BUILDING KANDINSKY ---------------------------------------------------------------------------
bazel build kandinsky:main %ARGS% || goto :build_error

@echo BUILDING APP DLL -----------------------------------------------------------------------------
bazel build apps/%APP_NAME%:%APP_NAME%_shared %ARGS% || goto :build_error

@goto :done

:build_error
@echo "BUILD FAILED!"
@exit /b 1

:done

