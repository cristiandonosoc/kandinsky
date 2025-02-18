@set SCRIPT_PATH=%~dp0
@pushd %SCRIPT_PATH%
bazel build kandinsky/apps:learn_opengl_shared --verbose_failures || goto :build_error
bazel build kandinsky:main --verbose_failures || goto :build_error

@goto :done

:build_error
@echo "BUILD FAILED!"

:done

