<p align="center">
  <img src="screenshots/Repo%20Banner.png" alt="Switchelf Banner" width="80%">
</p>

<h1 align="center">Switchelf</h1>
<p align="center"><em>Your personal bookshelf for the Nintendo Switch.</em></p>

<p align="center">
  <strong>Switch</strong> + <strong>Shelf</strong> = <strong>Switchelf</strong>
</p>

<p align="center">
  <img alt="Version" src="https://img.shields.io/badge/version-0.6.0--beta-blue?style=flat-square">
  <img alt="Platform" src="https://img.shields.io/badge/platform-Switch-red?style=flat-square">
  <img alt="License" src="https://img.shields.io/badge/license-GPL--3.0-green?style=flat-square">
</p>

---

## Features

- **Save your place** — automatically remembers the last page you read.
- **Broad format support** — PDF, EPUB, CBZ, and XPS.
- **Theme system** — customizable themes loaded from SD card (`/switch/Switchelf/themes/`). Dark and light defaults are bundled; drop a `theme.json` in a new folder to create your own.
- **Night mode overlay** — warm amber tint toggled with the Plus button for comfortable reading in low light.
- **Flexible orientation** — Landscape and portrait reading views, with rotation preference saved per-book.
- **Theme picker** — press Minus in the chooser or reader to open the theme picker modal and switch themes on the fly.
- **Touch controls** — tap the top/bottom of the screen to zoom in/out, and the left/right sides to turn pages.
- **Easy library management** — books are read from `/switch/Switchelf/books`.
- **JSON config** — single `config.json` stores all settings, saved pages, and preferences.

## Screenshots

<p align="center">
  <img src="screenshots/main%20menu.jpg" alt="Main Menu" width="45%">
  &nbsp;
  <img src="screenshots/landscape.jpg" alt="Landscape Reading" width="45%">
</p>

<p align="center">
  <img src="screenshots/portrait.jpg" alt="Portrait Reading" width="45%">
  &nbsp;
  <img src="screenshots/reading-black-mode.jpg" alt="Dark Mode Reading" width="45%">
</p>

## Building

### Prerequisites (Debian/Ubuntu)

1. **Install devkitPro**

   ```bash
   sudo apt install build-essential make pkg-config

   # Add devkitPro APT repository
   sudo mkdir -p /usr/share/keyring/
   sudo wget -U "dkp apt" -O /usr/share/keyring/devkitpro-pub.gpg https://apt.devkitpro.org/devkitpro-pub.gpg
   echo "deb [signed-by=/usr/share/keyring/devkitpro-pub.gpg] https://apt.devkitpro.org stable main" | sudo tee /etc/apt/sources.list.d/devkitpro.list
   sudo apt-get update

   # Install Switch toolchain and portlibs
   sudo apt-get install devkitpro-pacman
   sudo dkp-pacman -S switch-dev libnx switch-portlibs
   ```

2. **Build**

   ```bash
   export DEVKITPRO=/opt/devkitpro
   make
   ```

3. **Clean**

   ```bash
   make clean
   ```

The output `.nro` will be placed in `dist/Switchelf.nro`.

> **Note:** If you do not have the twili debugger installed, build with `NODEBUG=1`:
> ```bash
> NODEBUG=1 make
> ```

## Creating Themes

Themes are JSON files stored on your SD card at `/switch/Switchelf/themes/<theme_name>/theme.json`. Two themes (Dark and Light) are bundled and copied on first launch.

To create a custom theme:

1. Create a new folder under `/switch/Switchelf/themes/` (e.g. `pinkie/`)
2. Place a `theme.json` inside it
3. Open the theme picker (press **Minus**) and select your theme

### Theme file structure

```jsonc
{
    "name": "My Theme",                          // Display name in the picker

    "colors": {
        "background":     "FFF0F5",              // App background
        "text":           "4A148C",              // Primary text
        "accent":         "FF69B4",              // Highlights & links
        "page_bg":        "FFFFFF",              // Document page background
        "page_bg_light":  "FFF9FB",              // Alt page background
        "status_bar":     "FF1493",              // Status bar fill
        "selector":       "F8BBD0",              // Selection / active item bg
        "hint":           "F48FB1",              // Hint / secondary text
        "title":          "D81B60",              // Book titles & headings
        "separator":      "F06292",              // Divider lines
        "icon_tint":      "FFFFFF",              // Icon foreground color
        "icon_bg":        "00000000",            // Icon background (supports alpha)
        "badge_bg":       "FF80ABCC",            // Badge background (supports alpha)
        "badge_text":     "880E4FFF",            // Badge text (supports alpha)
        "bar_bg":         "FCE4EC",              // Progress bar background
        "separator_bar":  "F06292"               // Progress bar fill
    },

    "night_overlay": {
        "color": "FF1493",                       // Overlay tint color
        "alpha": 40                              // Overlay opacity (0-255)
    },

    "imgui": {
        // RGBA float arrays [R, G, B, A] — values from 0.0 to 1.0
        // Controls all ImGui UI elements (buttons, windows, scrollbars, etc.)
        "Text":                 [0.29, 0.08, 0.55, 1.00],
        "WindowBg":             [1.00, 0.96, 0.98, 1.00],
        "Button":               [1.00, 0.80, 0.90, 1.00],
        "ButtonHovered":        [1.00, 0.70, 0.85, 1.00],
        "ButtonActive":         [1.00, 0.41, 0.70, 1.00]
        // ... see bundled themes for the full list of supported keys
    }
}
```

### Color format

- **`colors`** — 6-digit hex (`RRGGBB`) or 8-digit hex with alpha (`RRGGBBAA`)
- **`night_overlay`** — 6-digit hex color + integer alpha (0 = transparent, 255 = opaque)
- **`imgui`** — `[R, G, B, A]` float array, each value from `0.0` to `1.0`

### Dark / Light detection

The app checks the `background` color luminance (`0.299R + 0.587G + 0.114B < 128`) to decide if the theme is dark or light. This affects icon rendering and other automatic adjustments.

### Example: Pinkie

```json
{
    "name": "Pinkie",
    "colors": {
        "background": "FFF0F5",
        "text": "4A148C",
        "accent": "FF69B4",
        "page_bg": "FFFFFF",
        "page_bg_light": "FFF9FB",
        "status_bar": "FF1493",
        "selector": "F8BBD0",
        "hint": "F48FB1",
        "title": "D81B60",
        "separator": "F06292",
        "icon_tint": "FFFFFF",
        "icon_bg": "00000000",
        "badge_bg": "FF80ABCC",
        "badge_text": "880E4FFF",
        "bar_bg": "FCE4EC",
        "separator_bar": "F06292"
    },
    "night_overlay": {
        "color": "FF1493",
        "alpha": 40
    },
    "imgui": {
        "Text":                 [0.29, 0.08, 0.55, 1.00],
        "TextDisabled":         [0.60, 0.40, 0.50, 1.00],
        "WindowBg":             [1.00, 0.96, 0.98, 1.00],
        "ChildBg":              [1.00, 1.00, 1.00, 1.00],
        "PopupBg":              [1.00, 1.00, 1.00, 0.98],
        "Border":               [1.00, 0.41, 0.70, 0.50],
        "FrameBg":              [1.00, 0.92, 0.95, 1.00],
        "FrameBgHovered":       [1.00, 0.85, 0.90, 1.00],
        "FrameBgActive":        [1.00, 0.75, 0.85, 1.00],
        "TitleBg":              [1.00, 0.80, 0.90, 1.00],
        "TitleBgActive":        [1.00, 0.41, 0.70, 1.00],
        "MenuBarBg":            [1.00, 0.95, 0.97, 1.00],
        "ScrollbarBg":          [1.00, 0.90, 0.95, 1.00],
        "ScrollbarGrab":        [1.00, 0.41, 0.70, 0.80],
        "ScrollbarGrabHovered": [1.00, 0.20, 0.60, 1.00],
        "ScrollbarGrabActive":  [0.85, 0.10, 0.50, 1.00],
        "CheckMark":            [1.00, 0.08, 0.58, 1.00],
        "SliderGrab":           [1.00, 0.41, 0.70, 1.00],
        "SliderGrabActive":     [1.00, 0.08, 0.58, 1.00],
        "Button":               [1.00, 0.80, 0.90, 1.00],
        "ButtonHovered":        [1.00, 0.70, 0.85, 1.00],
        "ButtonActive":         [1.00, 0.41, 0.70, 1.00],
        "Header":               [1.00, 0.85, 0.90, 1.00],
        "HeaderHovered":        [1.00, 0.75, 0.85, 1.00],
        "HeaderActive":         [1.00, 0.41, 0.70, 0.40],
        "Separator":            [1.00, 0.41, 0.70, 0.50],
        "ResizeGrip":           [1.00, 0.41, 0.70, 0.50],
        "ResizeGripHovered":    [1.00, 0.41, 0.70, 0.75],
        "ResizeGripActive":     [1.00, 0.08, 0.58, 1.00],
        "Tab":                  [1.00, 0.88, 0.92, 1.00],
        "TabHovered":           [1.00, 0.75, 0.85, 1.00],
        "TabActive":            [1.00, 0.41, 0.70, 1.00],
        "ModalWindowDimBg":     [0.30, 0.10, 0.20, 0.35]
    }
}
```

## Contributing

Contributions are welcome! Whether it's bug reports, feature suggestions, or pull requests, feel free to get involved. Fork the repository, make your changes, and open a PR — we'd love to have you help make Switchelf better.

## Credits

- **Areshk** — Maintainer and lead developer of Switchelf.
- **[SeanOMik](https://github.com/SeanOMik)** — Original author of [eBookReaderSwitch](https://github.com/SeanOMik/eBookReaderSwitch); this project is a fork of their work.
- **moronigranja** — For enabling additional file format support.
- **NX-Shell Team** — A good portion of the codebase is derived from an earlier version of their application.
- **[RapidJSON](https://rapidjson.org/)** — Used for JSON config and theme file parsing.

## License

This project is licensed under the GNU General Public License v3.0.
