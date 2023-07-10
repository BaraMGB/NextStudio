mkdir -p ./build/
cd build
rm CMakeCache.txt
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=RelWithDebInfo .. && cmake --build . && mv ~/Projects/NextSTudio/builds/compile_commands.json ~/Projects/NextSTudio/ && ./App/NextStudio_artefacts/RelWithDebInfo/NextStudio
cd ..
