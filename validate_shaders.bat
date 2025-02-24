@set SCRIPT_PATH=%~dp0
@pushd %SCRIPT_PATH%

@set VALIDATOR=extras\glsl\glslangValidator.exe
@if not exist %VALIDATOR% (
	echo Cannot find %VALIDATOR%
	exit /b 1
)
@echo Found validator at %VALIDATOR%

@for %%f in (assets\shaders\*) do @(
	@echo Validating %%f
	%VALIDATOR% %%f || goto :error
)

@goto :done

:error
@echo VALIDATION ERROR!
@exit /b 1

:done


