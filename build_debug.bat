@set SCRIPT_PATH=%~dp0
@pushd %SCRIPT_PATH%
@set ARGS=%*

@echo VALIDATING SHADERS ---------------------------------------------------------------------------
@call validate_shaders || goto :build_error

@echo BUILDING KANDINSKY ---------------------------------------------------------------------------
bazel build kandinsky:main %ARGS% || goto :build_error

@echo BUILDING APP DLL -----------------------------------------------------------------------------
bazel build apps/learn_opengl:learn_opengl_shared %ARGS% || goto :build_error

@goto :done

:build_error
@echo "BUILD FAILED!"
@exit /b 1

:done

