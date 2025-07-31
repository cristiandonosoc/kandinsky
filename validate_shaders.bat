@set SCRIPT_PATH=%~dp0
@pushd %SCRIPT_PATH%

@set VALIDATOR=extras\glsl\glslangValidator.exe
@if not exist %VALIDATOR% (
	echo Cannot find %VALIDATOR%
	exit /b 1
)
@echo Found validator at %VALIDATOR%

@for %%f in (assets\shaders\*.glsl) do @(
	@echo "Validating %%f (vertex)"
	%VALIDATOR% -l --glsl-version 430 -S vert -DVERTEX_SHADER %%f || goto :error
	@echo "Validating %%f (fragment)"
	%VALIDATOR% -l --glsl-version 430 -S frag -DFRAGMENT_SHADER %%f || goto :error
)

@goto :done

:error
@echo VALIDATION ERROR!
@exit /b 1

:done


