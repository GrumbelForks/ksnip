# AGENTS.md — ksnip

## Project

**ksnip** is a Qt-based cross-platform screenshot tool with annotation features
(active window, rectangular area, fullscreen, etc.). Upstream:
https://github.com/ksnip/ksnip

- License: GPL-2.0-or-later (`LICENSE.txt`)
- Language: C++17, CMake
- UI: Qt 5.15+ or Qt 6 (`-DBUILD_WITH_QT6=ON`)
- Core libs: [kImageAnnotator](https://github.com/ksnip/kImageAnnotator),
  [kColorPicker](https://github.com/ksnip/kColorPicker) (submodules under
  `libraries/` or system packages)

## Layout

| Path | Role |
|------|------|
| `src/backend/imageGrabber/` | Platform screenshot capture (X11, Gnome, KDE/Wayland, Windows, macOS) |
| `src/backend/config/` | Settings (`IConfig`, `Config`, `ConfigOptions`) |
| `src/gui/settingsDialog/` | Settings UI (e.g. `ImageGrabberSettings`) |
| `src/gui/snippingArea/` | Interactive region selection |
| `src/common/` | Shared helpers, enums, DTOs |
| `desktop/` | `.desktop` file |
| `translations/` | i18n |

Active-window geometry is resolved per platform via wrappers
(`X11Wrapper`, `GnomeX11Wrapper`, `WinWrapper`, …). On GNOME/X11,
`GnomeX11Wrapper` already subtracts `_GTK_FRAME_EXTENTS` so client area is
captured without the shadow/border region (see issue #460).

## Build (Nix)

```bash
nix build          # packages.default
nix develop        # dev shell with Qt6 + deps
```

Non-Nix:

```bash
cmake -B build -DBUILD_WITH_QT6=ON \
  -DUSE_SUBMODULE_KCOLORPICKER=OFF \
  -DUSE_SUBMODULE_KIMAGEANNOTATOR=OFF
cmake --build build -j
```

## Agent conventions

- Read **TODO.md** for current tip, open work, and handoff notes.
- Document plans and outcomes in **TODO.md** as you go.
- Prefer correct design over quick hacks; do not remove features without discussion.
- Existing project license applies (GPL-2.0-or-later); keep copyright headers consistent.
- Author for agent commits: `Ingo Ruhnke <grumbel@gmail.com>`
- Trailer: `Co-authored-by: Grok <grok@x.ai>`
- Deliver progress as **git bundles** under the session artifacts directory
  (full cumulative history from the work-line base; no patch files).

## Known focus areas

- Optional control for including/excluding window decorations (borders/shadows)
  in single-window captures — see **TODO.md**.
