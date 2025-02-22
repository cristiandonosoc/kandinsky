call build_debug.bat || goto :build_error
::bazel-bin\kandinsky\main.exe || goto :run_error
if exist "kandinsy.rdbg" (
    remedybg -q -g kandinsky.rdbg
) else (
    remedybg -q -g bazel-bin\kandinsky\main.exe
)

@goto :done

:build_error
:: We already logged an error message.
@goto :done

:run_error
@echo "RUN ERROR!"

:done



