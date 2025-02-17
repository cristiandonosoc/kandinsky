@set SCRIPT_PATH=%~dp0
@pushd %SCRIPT_PATH%
bazel build kandinsky/apps:learn_opengl_shared --verbose_failures || goto :error
bazel build kandinsky:main --verbose_failures || goto :error
bazel-bin\kandinsky\main.exe || goto :error
@goto :ok

:error
@echo "ERROR!"

:ok

