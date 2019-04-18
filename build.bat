@echo off

set FLAGS=-Zi -W3 -FC -GR- -EHa- -fp:except -nologo -Zf

pushd build\debug

cl %FLAGS% ..\..\main.c

popd