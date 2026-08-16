# Framebuffer

Render a quad into an offscreen color/depth framebuffer, read its RGBA pixels,
write the result to `framebuffer.png`, then restore the previous render target
and viewport. `bind()`/`unbind()` are stack-aware. The image is saved under
`build-examples/examples/output/10_framebuffer/`. VShade currently exposes the
attachment ID for advanced native use; a first-class attachment `Texture2D`
view is planned. Next: `11_cube`.
