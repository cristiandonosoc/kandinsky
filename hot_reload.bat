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

@echo BUILDING APP DLL -----------------------------------------------------------------------------
bazel build apps/%APP_NAME%:%APP_NAME%_shared %ARGS% || goto :error

@echo VALIDATING SHADERS ---------------------------------------------------------------------------
@call validate_shaders || goto :error
touch SHADER_MARKER

@goto :done

:error
@echo "HOT UPDATE FAILED!"
@exit /b 1

:done

