# Web version of VCMI

This fail created to cover special steps needed to build web version.
Current cmake build supports only linux environment.

1. You need to install [emsdk](https://github.com/emscripten-core/emsdk)
2. Install emscripten following emsdk guide
3. Create directory `emscripten` in vcmi root and cd into it
4. Use emsdk and cmake to configure build script:

    ```
    emcmake cmake -GNinja -DCMAKE_BUILD_TYPE=MinSizeRel ..
    ```
   
    **NOTE**: Long story short, you must use MinSizeRel for optimal performance

5. Build some ports required by vcmi (this step is not required but because of emscripten bug you should do it before building vcmi)
   
   ```
   embuilder build sdl2 sdl2_ttf sdl2_image sdl2_mixer
   ```
   
6. Now build the required libraries

   ```
   ninja boost-lib
   ninja tbb-lib
   ninja vcmi-extras
   ```
   
7. Finally is time to build vcmiclient.js:

   ```
   ninja -j<n> html5
   ```
   
   **NOTE**: Compilation will fail on boost/qvm/quat_traits.hpp, just add <Q> in problematic lines.

Now your build is in emscripten/html5 folder:

* **vcmiclient.[js,wasm]** - the game itself
* **vcmidata.[data,data.js]** - data files required by game + vcmi-extras

