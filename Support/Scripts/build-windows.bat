::
:: Copyright (c) 2014-present, Facebook, Inc.
:: All rights reserved.
::
:: This source code is licensed under the University of Illinois/NCSA Open
:: Source License found in the LICENSE file in the root directory of this
:: source tree. An additional grant of patent rights can be found in the
:: PATENTS file in the same directory.
::

:: After creating a build directory and running cmake, just run this script to
:: build the project with msbuild. If running from powershell, use
::     ..\Support\Scripts\build-windows.bat

@echo off

set vs2017devcmd=C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\Common7\Tools\vsdevcmd.bat
if exist "%vs2017devcmd%" (
    call "%vs2017devcmd%" -no_logo
) else (
    call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\Common7\tools\vsvars32.bat"
)
msbuild /nologo /verbosity:minimal /p:Configuration=Release ds2.vcxproj
