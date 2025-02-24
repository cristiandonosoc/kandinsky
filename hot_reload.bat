@set SCRIPT_PATH=%~dp0
@pushd %SCRIPT_PATH%
@set ARGS=%*

@echo BUILDING APP DLL -----------------------------------------------------------------------------
bazel build apps/learn_opengl:learn_opengl_shared %ARGS% || goto :error

@echo VALIDATING SHADERS ---------------------------------------------------------------------------
@call validate_shaders || goto :error
touch SHADER_MARKER

:error
@echo "HOT UPDATE FAILED!"
@exit /b 1

:done

