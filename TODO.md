# TODO.md — agent handoff

## Tip / base

- Work-line **base**: `059f022` (`059f022cb9c4bdd30d3e8311eb9199ca92fed4f3`) — upstream `master` at start of this sequence.
- Current tip: see `git log -1` after applying the latest bundle (X11 include-window-decoration option).

## Done

1. **flake.nix** + **flake.lock** — Nix packaging (Qt6, nixpkgs kimageannotator/kcolorpicker).
2. **AGENTS.md** — project overview and agent conventions.
3. **X11: option to exclude window decorations from single-window screenshots**
   - Config: `includeWindowDecoration()` (default **false** = client area only).
   - Settings UI checkbox: "Include window decorations" under Image Grabber.
   - `X11Wrapper::getFrameExtents` reads `_NET_FRAME_EXTENTS`, falls back to `_GTK_FRAME_EXTENTS`.
   - `BaseX11ImageGrabber::activeWindowRect()` subtracts extents when the option is off.
   - `GnomeX11Wrapper` no longer hard-codes GTK extent subtraction (shared path).

## Remaining / follow-ups

- Manual test on X11: GNOME, MATE/XFCE, KDE — active window with option on/off.
- Windows: optional client-rect vs `DWMWA_EXTENDED_FRAME_BOUNDS` branch using the same config key.
- Wayland: document as best-effort / no-op (compositor-controlled).
- CHANGELOG entry + translation string for the new checkbox (Weblate).

## Bundle naming

- Base short: `059f022`
- Next bundle: continue numbering; do not reuse an earlier NNN.
