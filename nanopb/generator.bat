cd /d %~dp0

:: pip install --upgrade protobuf grpcio-tools
python ../../../.pio/libdeps/esp12e/Nanopb/generator/nanopb_generator.py config.proto

move /Y config.pb.h ..\include
move /Y config.pb.c ..\src
