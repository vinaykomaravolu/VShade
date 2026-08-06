# VShade API reference

The generated API reference is published at
[vinaykomaravolu.github.io/VShade](https://vinaykomaravolu.github.io/VShade/).

Pull requests targeting `main` build the same documentation as a validation
check. Pushes to `main` publish the successful build to GitHub Pages.

## Build locally

Doxygen 1.9 or newer is required. Configure the documentation option and build
the dedicated target:

```sh
cmake -S . -B build-docs -DVSHADE_BUILD_DOCS=ON -DVSHADE_BUILD_SANDBOX=OFF -DBUILD_TESTING=OFF
cmake --build build-docs --target VShadeDocs
```

Open `build-docs/docs/html/index.html` in a browser after the target finishes.
