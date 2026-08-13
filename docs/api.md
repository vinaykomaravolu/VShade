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

## Header comment style

Public declarations under `engine/include` use Doxygen comments. Start with a
short `@brief`, then include only the caller-facing tags that apply:

```cpp
/**
 * @brief Creates a framebuffer with the specified dimensions.
 * @param width Width of the framebuffer in pixels.
 * @param height Height of the framebuffer in pixels.
 * @throws std::invalid_argument If either dimension is zero.
 * @note The renderer must be initialized before construction.
 */
Framebuffer(std::uint32_t width, std::uint32_t height);
```

- Use `@param` for every meaningful named parameter.
- Use `@return` for non-void results.
- Use `@throws` for exceptions callers can reasonably encounter.
- Use `@note` for important behavior or preconditions.
- Use `@warning` for dangerous lifetime rules or easy-to-misuse inputs.

The documentation build enables Doxygen's missing-parameter warnings and
treats documentation warnings as errors.
