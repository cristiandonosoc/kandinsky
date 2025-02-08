@set SCRIPT_PATH=%~dp0
@pushd %SCRIPT_PATH%
bazel build kandinsky:main --verbose_failures || goto :error
bazel-bin\kandinsky\main.exe || goto :error
@goto :ok

:error
@echo "ERROR!"

:ok

