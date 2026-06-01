# Raymark

**Raymark** — cross‑platform GPU benchmark.

Renders an instanced field of 3D spikes (256×256 = 65,536) animated via shaders, arranged into the **π** symbol. Overlay shows real‑time FPS metrics: average, 1% low, and 0.1% low.

Built with [raylib-iOS](https://github.com/ghera/raylib-iOS) (a raylib fork that compiles on all platforms including iOS).

Based on **[pi-field](https://github.com/letsreinventthewheel/pi-field)** by [letsreinventthewheel](https://github.com/letsreinventthewheel/pi-field) — a single‑mesh animated spike field driven entirely by shaders. This project takes pi-field's effect and wraps it in a cross‑platform raylib prject with performance overlay and mmobile support (Android and iOS).

## Benchmark Results

| Device | GPU | Screen |
|--------|-----|--------|
| <a href="results/Mac%20mini%20M4.png"><img src="results/Mac%20mini%20M4.png" alt="Mac mini M4" width="240"></a> | M4 (10-core GPU) | 1920×1200 |
| <a href="results/AMD%20Ryzen%207800X3D%20%2B%20NVIDIA%20GeForce%20RTX%204070%20Super.png"><img src="results/AMD%20Ryzen%207800X3D%20%2B%20NVIDIA%20GeForce%20RTX%204070%20Super.png" alt="AMD Ryzen 7800X3D + NVIDIA GeForce RTX 4070 Super" width="240"></a> | NVIDIA GeForce RTX 4070 SUPER | 1920×1080 |
| <a href="results/iPad%20Air%205th%20M1.png"><img src="results/iPad%20Air%205th%20M1.png" alt="iPad Air 5th M1" width="240"></a> | M1 (8-core GPU) | 2360×1640 |
| <a href="results/iPhone%2015.png"><img src="results/iPhone%2015.png" alt="iPhone 15" width="240"></a> | A16 Bionic | 2556×1179 |
| <a href="results/Samsung%20Galaxy%20A56.png"><img src="results/Samsung%20Galaxy%20A56.png" alt="Samsung Galaxy A56" width="240"></a> | Exynos 1480 | 2340×1080 |

## Resolution

- **Desktop**: windowed at 720p (1280×720), fullscreen (Alt + Enter) at monitor resolution
- **Mobile** (Android/iOS): fullscreen at native device resolution

The spike grid is 256×256 at the 720p reference height. It scales automatically with resolution so the visual density stays consistent across devices.

## Features

- 65,536 instanced 3D spikes with per‑spike vertex/fragment shader animation
- π symbol mask carved into the field
- Animated gradient background
- Orbit camera auto‑rotation
- FPS overlay: average, 1% low, 0.1% low
- GPU, OpenGL, and raylib version info
- Cross‑platform: macOS, Windows, Linux, Android, iOS

## Supported Platforms

| Platform | Status |
|----------|--------|
| macOS    | ✅ |
| Linux    | ✅ |
| Windows  | ✅ (MSVC + MinGW) |
| Android  | ✅ |
| iOS      | ✅ |

## Quick Build (Desktop)

### macOS / Linux

```sh
cd desktop/build
chmod +x premake5.osx   # Linux: premake5
./premake5.osx gmake2
cd ..
make
./bin/Debug/desktop
```

### VSCode

Open the `desktop/` folder in VSCode and run the **build debug** task (`Ctrl+Shift+B` or `Cmd+Shift+B`). Works on macOS, Linux, and Windows.

### Xcode

Open the `ios/` folder in Xcode and build/run.

### Android Studio

Open the `android/` folder in Android Studio and build/run.

## Performance Metrics

The benchmark shows three FPS values:

- **Avg** — average FPS over a 1000‑frame window
- **1% low** — average of the worst 1% of frames (smoothness indicator)
- **0.1% low** — average of the worst 0.1% of frames (stutter indicator)

Data collection starts after 3 seconds of warmup. Frame times are sorted for correct percentile calculation.

## Project Structure

```
raymark/
├── desktop/          # desktop sources (C, raylib)
│   ├── src/core/     # main, spike_field, background, shader_utils
│   └── resources/    # GLSL shaders, π mask
├── ios/              # Xcode iOS project
├── android/          # Android project (gradle + NDK)
├── raylib/           # raylib submodule
└── README.md
```

## Credits

This project is a direct copy, with some additions, of **[pi-field](https://github.com/letsreinventthewheel/pi-field)** by [letsreinventthewheel](https://github.com/letsreinventthewheel/pi-field).

## License

MIT © 2025 ghera (https://github.com/ghera/raymark)

See [LICENSE](./LICENSE).
