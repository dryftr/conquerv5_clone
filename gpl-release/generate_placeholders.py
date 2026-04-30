#!/usr/bin/env python3
"""
Generate placeholder sprite PNGs for Conquer V5
Phase 5b: Placeholder Art Generation
"""

import os
import sys

try:
    from PIL import Image, ImageDraw
except ImportError:
    print("PIL/Pillow not available. Install with: pip install Pillow")
    sys.exit(1)

SPRITE_DIR = "./sprites"
SIZE = 32

# Terrain elevation colors (matching SDL2 renderer)
TERRAIN = {
    "water": (0, 100, 200),
    "valley": (144, 238, 144),
    "clear": (128, 128, 128),
    "hill": (139, 69, 19),
    "mountain": (105, 105, 105),
    "peak": (112, 128, 144),
}

# Vegetation colors
VEGETATION = {
    "volcano": (255, 69, 0),
    "desert": (244, 164, 96),
    "tundra": (230, 230, 250),
    "barren": (92, 64, 51),
    "lt_veg": (152, 251, 152),
    "good": (34, 139, 34),
    "wood": (189, 183, 107),
    "forest": (0, 100, 0),
    "jungle": (50, 205, 50),
    "swamp": (0, 128, 128),
    "ice": (173, 216, 230),
    "none": (0, 0, 0),
}

# Buildings
BUILDINGS = {
    "farm": (210, 180, 140),
    "city": (100, 100, 150),
    "capital": (150, 100, 100),
    "mine": (80, 60, 40),
    "town": (120, 120, 120),
}

# UI
UI = {
    "cursor": (255, 255, 0),
    "selection": (0, 255, 255),
}

def draw_hexagon(draw, center, size, color, outline=None):
    """Draw a pointy-top hexagon"""
    import math
    points = []
    for i in range(6):
        angle_deg = 60 * i - 30
        angle_rad = math.radians(angle_deg)
        x = center[0] + size * math.cos(angle_rad)
        y = center[1] + size * math.sin(angle_rad)
        points.append((x, y))
    
    draw.polygon(points, fill=color, outline=outline)

def generate_terrain_sprite(name, color):
    """Generate terrain elevation sprite with hex shape"""
    img = Image.new('RGBA', (SIZE, SIZE), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    center = (SIZE // 2, SIZE // 2)
    hex_size = SIZE // 2 - 2
    
    # Draw hexagon
    draw_hexagon(draw, center, hex_size, color, (50, 50, 50))
    
    # Add label
    # draw.text((4, SIZE//2-4), name[:2].upper(), fill=(255, 255, 255))
    
    return img

def generate_vegetation_sprite(name, color):
    """Generate vegetation sprite"""
    img = Image.new('RGBA', (SIZE, SIZE), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    center = (SIZE // 2, SIZE // 2)
    
    # Different shapes for different vegetation
    if name == "volcano":
        # Triangle for volcano
        points = [(center[0], 4), (center[0]-10, SIZE-4), (center[0]+10, SIZE-4)]
        draw.polygon(points, fill=color, outline=(150, 50, 0))
    elif name == "forest":
        # Tree-like circles
        draw.ellipse([center[0]-6, 4, center[0]+6, 16], fill=color)
        draw.ellipse([center[0]-10, 12, center[0]-2, 22], fill=color)
        draw.ellipse([center[0]+2, 12, center[0]+10, 22], fill=color)
    elif name == "mountain":
        # Triangle
        points = [(center[0], 4), (center[0]-12, SIZE-4), (center[0]+12, SIZE-4)]
        draw.polygon(points, fill=color, outline=(100, 100, 100))
    else:
        # Default: circle or small shape
        draw.ellipse([8, 8, SIZE-8, SIZE-8], fill=color, outline=(50, 50, 50))
    
    return img

def generate_building_sprite(name, color):
    """Generate building sprite"""
    img = Image.new('RGBA', (SIZE, SIZE), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    if name == "city":
        # Building silhouette
        draw.rectangle([8, 8, SIZE-8, SIZE-8], fill=color, outline=(200, 200, 200))
        # Roof
        draw.polygon([(8, 8), (SIZE//2, 2), (SIZE-8, 8)], fill=(150, 150, 150))
    elif name == "capital":
        # Larger building with flag
        draw.rectangle([6, 10, SIZE-6, SIZE-6], fill=color, outline=(200, 200, 200))
        draw.rectangle([SIZE-10, 4, SIZE-8, 10], fill=(200, 50, 50))  # Flag
    else:
        # Simple square
        draw.rectangle([8, 8, SIZE-8, SIZE-8], fill=color, outline=(100, 100, 100))
    
    return img

def generate_ui_sprite(name, color):
    """Generate UI sprite"""
    img = Image.new('RGBA', (SIZE, SIZE), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    if name == "cursor":
        # Crosshair
        draw.line([(SIZE//2, 0), (SIZE//2, SIZE)], fill=color, width=2)
        draw.line([(0, SIZE//2), (SIZE, SIZE//2)], fill=color, width=2)
        draw.ellipse([SIZE//2-4, SIZE//2-4, SIZE//2+4, SIZE//2+4], outline=color, width=2)
    elif name == "selection":
        # Highlight box
        draw.rectangle([4, 4, SIZE-4, SIZE-4], outline=color, width=3)
    
    return img

def main():
    """Generate all placeholder sprites"""
    print("=== Generating Placeholder Sprites ===")
    
    # Generate terrain
    for name, color in TERRAIN.items():
        path = f"{SPRITE_DIR}/terrain/elevation/{name}.png"
        img = generate_terrain_sprite(name, color + (255,))
        img.save(path)
        print(f"  Generated: {path}")
    
    # Generate vegetation
    for name, color in VEGETATION.items():
        path = f"{SPRITE_DIR}/terrain/vegetation/{name}.png"
        img = generate_vegetation_sprite(name, color + (255,))
        img.save(path)
        print(f"  Generated: {path}")
    
    # Generate buildings
    for name, color in BUILDINGS.items():
        path = f"{SPRITE_DIR}/buildings/{name}.png"
        img = generate_building_sprite(name, color + (255,))
        img.save(path)
        print(f"  Generated: {path}")
    
    # Generate UI
    for name, color in UI.items():
        path = f"{SPRITE_DIR}/ui/{name}.png"
        img = generate_ui_sprite(name, color + (255,))
        img.save(path)
        print(f"  Generated: {path}")
    
    # Create sprites.json for animation config
    sprites_json = '''{
  "terrain": {
    "elevation": {
      "water": { "frames": 1, "speed": 250, "size": [32, 32] }
    },
    "vegetation": {
      "volcano": { "frames": 1, "speed": 150, "size": [32, 32] },
      "forest": { "frames": 1, "speed": 200, "size": [32, 32] },
      "jungle": { "frames": 1, "speed": 180, "size": [32, 32] }
    }
  }
}'''
    
    with open(f"{SPRITE_DIR}/sprites.json", "w") as f:
        f.write(sprites_json)
    print(f"  Generated: {SPRITE_DIR}/sprites.json")
    
    print(f"\n=== Done! Generated {len(TERRAIN) + len(VEGETATION) + len(BUILDINGS) + len(UI)} sprites ===")
    print("\nTo test:")
    print("  make test-hexmap-sprites")
    print("  ./test_hexmap_sprites")

if __name__ == "__main__":
    main()
