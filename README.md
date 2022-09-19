# AAGEStarterTemplate
An easy to use template project for ActuallyAGameEngine https://github.com/ZackMason/ActuallyAGameEngine

This template was created using `cpp_init.py` https://github.com/ZackMason/cpp_init

# How To Build 

```
mkdir build
cd build
conan install .. -s build_type=Release
cmake ..
cmake --build . --config Release
```

follow conan instructions if this is your first time building AAGE, you probably need to add --build=missing
