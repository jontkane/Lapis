SET vcpkg=C:\vcpkg
SET lapis=C:\lapis
SET triplet=x64-windows

RMDIR /S /Q %lapis%\build
mkdir %lapis%\build
pushd %lapis%\build
mkdir bin
mkdir bin\Release
mkdir bin\Debug
mkdir bin\RelWithDebInfo
mkdir bin\MinSizeRel

COPY %vcpkg%\installed\%triplet%\share\proj\proj.db bin\Release\proj.db
COPY %vcpkg%\installed\%triplet%\share\proj\proj.ini bin\Release\proj.ini

COPY %vcpkg%\installed\%triplet%\share\proj\proj.db bin\Debug\proj.db
COPY %vcpkg%\installed\%triplet%\share\proj\proj.ini bin\Debug\proj.ini

COPY %vcpkg%\installed\%triplet%\share\proj\proj.db bin\RelWithDebInfo\proj.db
COPY %vcpkg%\installed\%triplet%\share\proj\proj.ini bin\RelWithDebInfo\proj.ini

COPY %vcpkg%\installed\%triplet%\share\proj\proj.db bin\MinSizeRel\proj.db
COPY %vcpkg%\installed\%triplet%\share\proj\proj.ini bin\MinSizeRel\proj.ini

cmake %lapis% -DCMAKE_TOOLCHAIN_FILE=%vcpkg%\scripts\buildsystems\vcpkg.cmake -DVCPKG_TARGET_TRIPLET=%triplet%
popd