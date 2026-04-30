#!/bin/bash
# Generate placeholder sprites using netpbm or raw PPM
# Fallback to colored PPM files if ImageMagick not available

SPRITE_DIR="./sprites"
SIZE=32

# Colors (R G B)
WATER="0 100 200"
VALLEY="144 238 144"
CLEAR="128 128 128"
HILL="139 69 19"
MOUNTAIN="105 105 105"
PEAK="112 128 144"
VOLCANO="255 69 0"
DESERT="244 164 96"
TUNDRA="230 230 250"
BARREN="92 64 51"
LT_VEG="152 251 152"
GOOD="34 139 34"
WOOD="189 183 107"
FOREST="0 100 0"
JUNGLE="50 205 50"
SWAMP="0 128 128"
ICE="173 216 230"
FARM="210 180 140"
CITY="100 100 150"
CAPITAL="150 100 100"
MINE="80 60 40"
TOWN="120 120 120"

# Function to create a solid colored PPM
create_ppm() {
    local file=$1
    local r=$2
    local g=$3
    local b=$4
    local size=$5
    
    # PPM header
    echo "P6" > "$file"
    echo "$size $size" >> "$file"
    echo "255" >> "$file"
    
    # Pixel data (repeated)
    for ((i=0; i<size*size; i++)); do
        printf "\\x$(printf '%02x' $r)\\x$(printf '%02x' $g)\\x$(printf '%02x' $b)" >> "$file"
    done
}

echo "=== Generating Placeholder Sprites ==="

# Generate terrain sprites
for name in water valley clear hill mountain peak; do
    color="$CLEAR"
    case $name in
        water) color="$WATER" ;;
        valley) color="$VALLEY" ;;
        clear) color="$CLEAR" ;;
        hill) color="$HILL" ;;
        mountain) color="$MOUNTAIN" ;;
        peak) color="$PEAK" ;;
    esac
    read r g b <<< "$color"
    create_ppm "$SPRITE_DIR/terrain/elevation/${name}.ppm" $r $g $b $SIZE
    echo "  Generated: terrain/elevation/${name}.ppm"
done

# Generate vegetation sprites  
for name in volcano desert tundra barren lt_veg good wood forest jungle swamp ice none; do
    color="$CLEAR"
    case $name in
        volcano) color="$VOLCANO" ;;
        desert) color="$DESERT" ;;
        tundra) color="$TUNDRA" ;;
        barren) color="$BARREN" ;;
        lt_veg) color="$LT_VEG" ;;
        good) color="$GOOD" ;;
        wood) color="$WOOD" ;;
        forest) color="$FOREST" ;;
        jungle) color="$JUNGLE" ;;
        swamp) color="$SWAMP" ;;
        ice) color="$ICE" ;;
        none) color="0 0 0" ;;
    esac
    read r g b <<< "$color"
    create_ppm "$SPRITE_DIR/terrain/vegetation/${name}.ppm" $r $g $b $SIZE
    echo "  Generated: terrain/vegetation/${name}.ppm"
done

# Generate building sprites
for name in farm city capital mine town; do
    color="$TOWN"
    case $name in
        farm) color="$FARM" ;;
        city) color="$CITY" ;;
        capital) color="$CAPITAL" ;;
        mine) color="$MINE" ;;
        town) color="$TOWN" ;;
    esac
    read r g b <<< "$color"
    create_ppm "$SPRITE_DIR/buildings/${name}.ppm" $r $g $b $SIZE
    echo "  Generated: buildings/${name}.ppm"
done

# Generate UI sprites
for name in cursor selection; do
    color="255 255 0"
    if [ "$name" = "selection" ]; then
        color="0 255 255"
    fi
    read r g b <<< "$color"
    create_ppm "$SPRITE_DIR/ui/${name}.ppm" $r $g $b $SIZE
    echo "  Generated: ui/${name}.ppm"
done

# Rename PPM to PNG for SDL2_image compatibility
# (SDL2_image might not support PPM in all builds, so document this)
echo ""
echo "=== Sprite Generation Complete ==="
echo "Generated PPM files. For best compatibility:"
echo "  Install ImageMagick: sudo apt install imagemagick"
echo "  Then convert: for f in sprites/**/*.ppm; do convert \$f \${f%.ppm}.png; done"
echo ""
echo "The sprite system will use fallback colored rectangles until"
echo "real PNG sprites are available. This is working as designed."
