# Render to Texture

Render into a framebuffer-backed color texture and restore the prior target.
The current API exposes `colorAttachmentId()` for advanced native integration;
a safe `Texture2D` attachment view is the next API step.
