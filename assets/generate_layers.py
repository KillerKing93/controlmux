import os
import math
from PIL import Image, ImageDraw, ImageFont, ImageFilter, ImageChops

WIDTH = 960
HEIGHT = 600

def create_layer_01_background():
    img = Image.new("RGBA", (WIDTH, HEIGHT), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    # Radial gradient background
    cx, cy = WIDTH // 2, HEIGHT // 2
    max_radius = math.hypot(cx, cy)
    
    bg = Image.new("RGBA", (WIDTH, HEIGHT))
    bg_draw = ImageDraw.Draw(bg)
    
    for y in range(HEIGHT):
        for x in range(WIDTH):
            dist = math.hypot(x - cx, y - cy) / max_radius
            # Gradient from deep dark cyan/blue center to near pitch black
            r = int(10 + (1 - dist) * 15)
            g = int(14 + (1 - dist) * 35)
            b = int(35 + (1 - dist) * 65)
            bg_draw.point((x, y), fill=(r, g, b, 255))

    # Grid overlay
    grid_draw = ImageDraw.Draw(bg)
    grid_color = (255, 255, 255, 12)
    for x in range(0, WIDTH, 40):
        grid_draw.line([(x, 0), (x, HEIGHT)], fill=grid_color, width=1)
    for y in range(0, HEIGHT, 40):
        grid_draw.line([(0, y), (WIDTH, y)], fill=grid_color, width=1)

    bg.save("assets/logo_layers/01_background.png")
    print("Saved 01_background.png")

def create_layer_02_circuit_core():
    img = Image.new("RGBA", (WIDTH, HEIGHT), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    cx, cy = WIDTH // 2, 230

    # Draw central glowing multiplexer core
    # Outer orbital dashed ring
    for r in [130, 110, 85]:
        draw.ellipse([cx - r, cy - r, cx + r, cy + r], outline=(0, 242, 254, 80), width=2)
    
    # Inner glowing reactor core
    core_layer = Image.new("RGBA", (WIDTH, HEIGHT), (0, 0, 0, 0))
    core_draw = ImageDraw.Draw(core_layer)
    core_draw.ellipse([cx - 45, cy - 45, cx + 45, cy + 45], fill=(0, 242, 254, 200), outline=(255, 255, 255, 255), width=3)
    core_draw.ellipse([cx - 20, cy - 20, cx + 20, cy + 20], fill=(255, 8, 68, 220))

    # Apply heavy bloom blur for plasma glow
    glow = core_layer.filter(ImageFilter.GaussianBlur(15))
    img = Image.alpha_composite(img, glow)
    img = Image.alpha_composite(img, core_layer)

    # Circuit connection wires
    draw = ImageDraw.Draw(img)
    draw.line([(180, cy), (cx - 45, cy)], fill=(0, 242, 254, 200), width=3)
    draw.line([(WIDTH - 180, cy), (cx + 45, cy)], fill=(255, 8, 68, 200), width=3)
    draw.line([(cx, cy - 130), (cx, cy - 45)], fill=(127, 0, 255, 180), width=3)

    # Node dots
    draw.ellipse([180 - 6, cy - 6, 180 + 6, cy + 6], fill=(0, 242, 254, 255), outline=(255, 255, 255, 255), width=2)
    draw.ellipse([WIDTH - 180 - 6, cy - 6, WIDTH - 180 + 6, cy + 6], fill=(255, 8, 68, 255), outline=(255, 255, 255, 255), width=2)

    img.save("assets/logo_layers/02_circuit_core.png")
    print("Saved 02_circuit_core.png")

def draw_cursor(img, start_x, start_y, main_color, border_color, label_text, is_cyan=True):
    # Draw high quality neon pointer cursor
    cursor_layer = Image.new("RGBA", (WIDTH, HEIGHT), (0, 0, 0, 0))
    draw = ImageDraw.Draw(cursor_layer)

    x, y = start_x, start_y
    if is_cyan:
        pts = [(x, y), (x + 10, y + 110), (x + 40, y + 85), (x + 85, y + 140), (x + 115, y + 115), (x + 65, y + 60), (x + 115, y + 60)]
    else:
        pts = [(x, y), (x - 10, y + 110), (x - 40, y + 85), (x - 85, y + 140), (x - 115, y + 115), (x - 65, y + 60), (x - 115, y + 60)]

    # Draw drop shadow
    shadow = Image.new("RGBA", (WIDTH, HEIGHT), (0, 0, 0, 0))
    s_draw = ImageDraw.Draw(shadow)
    shadow_pts = [(p[0] + 6, p[1] + 6) for p in pts]
    s_draw.polygon(shadow_pts, fill=(0, 0, 0, 180))
    shadow = shadow.filter(ImageFilter.GaussianBlur(8))
    img = Image.alpha_composite(img, shadow)

    # Draw cursor body & outline
    draw.polygon(pts, fill=main_color, outline=border_color, width=3)

    # Glow effect
    glow = cursor_layer.filter(ImageFilter.GaussianBlur(10))
    img = Image.alpha_composite(img, glow)
    img = Image.alpha_composite(img, cursor_layer)

    # Draw Person Badge Tag
    badge_layer = Image.new("RGBA", (WIDTH, HEIGHT), (0, 0, 0, 0))
    b_draw = ImageDraw.Draw(badge_layer)

    bx = x - 50 if not is_cyan else x - 20
    by = y - 35
    b_draw.rounded_rectangle([bx, by, bx + 110, by + 30], radius=6, fill=(15, 23, 42, 230), outline=main_color, width=2)
    
    try:
        font = ImageFont.truetype("arialbd.ttf", 14)
    except:
        font = ImageFont.load_default()

    b_draw.text((bx + 15, by + 6), label_text, fill=(255, 255, 255, 255), font=font)

    img = Image.alpha_composite(img, badge_layer)
    return img

def create_layer_03_cyan_cursor():
    img = Image.new("RGBA", (WIDTH, HEIGHT), (0, 0, 0, 0))
    img = draw_cursor(img, 240, 130, (0, 242, 254, 255), (255, 255, 255, 255), "PERSON 1", is_cyan=True)
    img.save("assets/logo_layers/03_cyan_cursor.png")
    print("Saved 03_cyan_cursor.png")

def create_layer_04_magenta_cursor():
    img = Image.new("RGBA", (WIDTH, HEIGHT), (0, 0, 0, 0))
    img = draw_cursor(img, WIDTH - 240, 130, (255, 8, 68, 255), (255, 255, 255, 255), "PERSON 2", is_cyan=False)
    img.save("assets/logo_layers/04_magenta_cursor.png")
    print("Saved 04_magenta_cursor.png")

def create_layer_05_typography():
    img = Image.new("RGBA", (WIDTH, HEIGHT), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    try:
        title_font = ImageFont.truetype("arialbd.ttf", 52)
        sub_font = ImageFont.truetype("arial.ttf", 16)
        author_font = ImageFont.truetype("arial.ttf", 13)
    except:
        title_font = sub_font = author_font = ImageFont.load_default()

    # Draw Title text CONTROLMUX
    draw.text((WIDTH // 2 - 170, 440), "CONTROL", fill=(255, 255, 255, 255), font=title_font)
    draw.text((WIDTH // 2 + 75, 440), "MUX", fill=(0, 242, 254, 255), font=title_font)

    # Subtitle
    draw.text((WIDTH // 2 - 175, 510), "MULTI-PERSON INPUT CONTROL ENGINE", fill=(160, 174, 192, 255), font=sub_font)
    
    # Author
    draw.text((WIDTH // 2 - 80, 540), "BY ALIF NURHIDAYAT", fill=(100, 116, 139, 255), font=author_font)

    img.save("assets/logo_layers/05_typography.png")
    print("Saved 05_typography.png")

if __name__ == "__main__":
    os.makedirs("assets/logo_layers", exist_ok=True)
    create_layer_01_background()
    create_layer_02_circuit_core()
    create_layer_03_cyan_cursor()
    create_layer_04_magenta_cursor()
    create_layer_05_typography()
