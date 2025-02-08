@set SCRIPT_PATH=%~dp0
@pushd %SCRIPT_PATH%
bazel build kandinsky:main --verbose_failures
bazel-bin\kandinsky\main.exe

