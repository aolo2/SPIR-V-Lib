@echo off

set FLAGS=-Zi -W2  -FC -GR- -EHa- -fp:except -nologo -Zf

pushd build\debug

cl %FLAGS% ..\..\newinst.c

popd