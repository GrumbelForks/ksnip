# TODO.md — agent handoff

## Tip / base

- Work-line **base**: `059f022` (`059f022cb9c4bdd30d3e8311eb9199ca92fed4f3`) — upstream `master` at start of this sequence.
- Current tip: same until first agent commit lands.

## Done this session

1. **flake.nix** — Nix flake packaging against Qt6 + nixpkgs `kimageannotator` / `kcolorpicker`; `packages.default` + `devShells.default`.
2. **AGENTS.md** — project overview, layout, build, agent conventions.
3. **Research**: option to exclude window borders from single-window screenshots (below).

## Research: disable window border in single-window screenshots

### Problem

Active-window captures often include decorations, shadows, or a non-transparent
“border” of background pixels around the client area. Users want a setting to
capture **client area only** (or to toggle inclusion of decorations).

Related upstream issues:

- [#460](https://github.com/ksnip/ksnip/issues/460) — Gnome: non-transparent border of background (addressed for GNOME via `_GTK_FRAME_EXTENTS`).
- [#912](https://github.com/ksnip/ksnip/issues/912) — MATE/X11 and some apps: large border with background still reported.

### Current behaviour

| Platform path | Active window rect | Border handling |
|---------------|--------------------|-----------------|
| `X11Wrapper` | Full geometry from `xcb_get_geometry` | No frame-extent subtraction |
| `GnomeX11Wrapper` | Geometry adjusted by `_GTK_FRAME_EXTENTS` | Always excludes GTK shadow/border extents |
| `WinWrapper` | `DwmGetWindowAttribute(..., DWMWA_EXTENDED_FRAME_BOUNDS, ...)` | Visible frame (includes borders) |
| Wayland portals / KDE / GNOME Wayland | Compositor-defined | Limited control |

Entry points:

- `AbstractRectAreaImageGrabber` → `CaptureModes::ActiveWindow` → `activeWindowRect()`
- `BaseX11ImageGrabber::activeWindowRect()` → `mX11Wrapper->getActiveWindowRect()`
- Settings UI: `ImageGrabberSettings` (cursor, hide main window, Wayland flags, delay) — **no border option yet**

### Recommended design

1. **Config key** (under image-grabber section), e.g.:
   - `includeWindowDecoration` (bool), default `true` for least surprise, or `false` if product decision is “content only”.
   - Pattern: `ConfigOptions::…String()`, `IConfig` getter/setter, `Config` load/save, checkbox in `ImageGrabberSettings`.

2. **Geometry adjustment** (when option is **false** = exclude borders):
   - Prefer standard **`_NET_FRAME_EXTENTS`** (EWMH) on X11 — works beyond GTK clients; left/right/top/bottom cardinals.
   - Keep **`_GTK_FRAME_EXTENTS`** as fallback or primary on GNOME (already implemented).
   - Windows: when excluding, use client rect (`GetClientRect` + map to screen) instead of `DWMWA_EXTENDED_FRAME_BOUNDS`.
   - Wayland: often impossible without compositor help; document as best-effort / no-op.

3. **Where to apply**:
   - Best: inside the wrapper `getActiveWindowRect()` implementations, with config injected or a shared helper used by `BaseX11ImageGrabber` / platform grabbers after obtaining the raw rect.
   - Avoid only adjusting in `GnomeX11Wrapper` so non-GNOME X11 (MATE, XFCE, etc.) benefits — that is the gap behind #912.

4. **UI copy**: checkbox under Image Grabber settings, e.g. “Include window decorations” (tooltip: title bar, borders, and shadows when the platform provides extents).

### Implementation sketch (not done)

- `IConfig` / `Config` / `ConfigOptions`: add `includeWindowDecoration()`.
- `ImageGrabberSettings`: checkbox + load/save.
- `X11Wrapper`: implement `getFrameExtents(windowId)` via `_NET_FRAME_EXTENTS`; apply in `getActiveWindowRect()` when config says exclude.
- `GnomeX11Wrapper`: either call shared helper or apply both atoms; respect the same config flag (today always subtracts).
- `WinWrapper`: branch on flag between extended frame bounds and client area.
- Pass `IConfig` into wrappers if they do not already hold it (today grabbers own config; wrappers are mostly free of it — inject or adjust one level up in `BaseX11ImageGrabber::activeWindowRect()`).

### Out of scope / risks

- Rounded corners / transparency of decorations (pixel fill policy) is separate from extents.
- Multi-monitor and HDPI: existing `mHdpiScaler` path must keep working after rect shrink.
- Do not change Wayland portal semantics without explicit compositor APIs.

## Next steps

1. Implement the config + X11 `_NET_FRAME_EXTENTS` path (highest impact for #912-class reports).
2. Wire Windows client-rect branch.
3. Manual test matrix: GNOME X11, MATE/XFCE X11, Windows; active window with/without decorations option.
4. Optional: unit-test extent parsing with a small xcb mock if tests already cover grabbers.

## Bundle naming

- Base short: `059f022`
- Next bundle: `ksnip-001.1-<slug>-059f022.bundle` (increment as needed).
