call build_debug.bat || goto :build_error
::bazel-bin\kandinsky\main.exe || goto :run_error
remedybg -q -g kandinsky.rdbg

@goto :done

:build_error
:: We already logged an error message.
@goto :done

:run_error
@echo "RUN ERROR!"

:done



