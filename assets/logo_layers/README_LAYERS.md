# ControlMux Modular Layered Logo Assets

The ControlMux logo graphics are built using a **multi-layer asset system** so you can freely edit, swap, or tweak any layer in **Photoshop, GIMP, Figma, Illustrator, or Canva**.

---

## 📂 Layer File Manifest

The logo consists of **5 independent transparent PNG layers** inside `assets/logo_layers/`:

| Layer File | Layer Name | Description |
| :--- | :--- | :--- |
| **`01_background.png`** | Background | Dark radial gradient with desktop grid lines |
| **`02_circuit_core.png`** | Circuit & Core | Multiplexer core, orbital rings & connection wires |
| **`03_cyan_cursor.png`** | Person 1 Cursor | 3D Neon Cyan pointer cursor & `PERSON 1` badge |
| **`04_magenta_cursor.png`** | Person 2 Cursor | 3D Neon Magenta pointer cursor & `PERSON 2` badge |
| **`05_typography.png`** | Brand & Typography | ControlMux title text & author attribution |

---

## 🛠️ How to Edit in Photoshop / GIMP / Canva / Figma

1. **Photoshop / GIMP / Canva**: Drag and drop all 5 PNG files from `assets/logo_layers/` onto a single canvas in order (1 at the bottom to 5 at the top).
2. **Vector SVG Source**: You can also open **`assets/logo.svg`** directly in **Adobe Illustrator / Inkscape / Figma**, where all elements are grouped as editable layers (`<g id="layer-01-background">`, etc.).
3. **Rebuilding `logo.png`**: After making any edits to individual layer files, simply run:
   ```bash
   python assets/merge_logo.py
   ```
   This will automatically re-composite all layers and update **`assets/logo.png`**!
