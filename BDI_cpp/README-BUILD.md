# Building BDI (scaffold)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
./build/bdi_hello
```
> The source tree expects `BDI_cpp/` as the C++ root (no spaces).
