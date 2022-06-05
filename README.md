# PPlusFilter
These are filters for PPlus application

## How to build
Open `PPlusVideoFilter.sln` with Visual Studio, change the active config to `Release` and the platform to `x64`, and build the solution.

The output can be found under `{ProjectFolder}\bin\x64\Release\PPlusVideoFilter.dll`.

If you have trouble building the solution after modifying/updating it, check whether all applications that may have used this filter has completely exited (eg. Zoom) in Task Manager. 

## How to use
To load the filter, open a terminal with administrative privilege, and run `regsvr32.exe {Path-to-the-built-DLL}`.

To unload the filter, open a terminal with administrative privilege, and run `regsvr32.exe /U {Path-to-the-built-DLL}`.
