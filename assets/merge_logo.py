"""
merge_logo.py
Composites all individual PNG layer files in assets/logo_layers/ into logo.png.
"""

import os
from PIL import Image

def merge_layers():
    layers_dir = os.path.join(os.path.dirname(__file__), "logo_layers")
    output_file = os.path.join(os.path.dirname(__file__), "logo.png")

    # Ordered list of layer files to merge from bottom to top
    layer_files = [
        "01_background.png",
        "02_circuit_core.png",
        "03_cyan_cursor.png",
        "04_magenta_cursor.png",
        "05_typography.png"
    ]

    base_img = None

    for filename in layer_files:
        path = os.path.join(layers_dir, filename)
        if os.path.exists(path):
            img = Image.open(path).convert("RGBA")
            if base_img is None:
                base_img = img
            else:
                base_img = Image.alpha_composite(base_img, img)
            print(f"Merged layer: {filename}")
        else:
            print(f"Warning: Layer file {filename} not found.")

    if base_img:
        base_img.save(output_file, "PNG")
        print(f"Successfully generated merged logo: {output_file}")

if __name__ == "__main__":
    merge_layers()
