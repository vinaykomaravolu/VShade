# Simple 2D Game: Block Runner

A tiny endless runner inspired by the browser dinosaur game. The blue block
moves right automatically while the orthographic camera follows and looks
ahead. Jump over the orange obstacles; obstacles that leave the camera are
recycled farther down the course.

## Controls

- **Space** or **Up**: jump while touching the floor
- **R**: restart after a collision
- **Escape**: exit

The example combines scene-driven rendering, a following camera, fixed-step
Box2D physics, collision callbacks, continuous collision detection, spatial
obstacle recycling, input, and an audio cue. Next: `24_simple_3d_game`.
