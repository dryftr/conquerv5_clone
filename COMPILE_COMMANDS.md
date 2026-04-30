# COMPILE_COMMANDS.md
# Quick reference for building Conquer V5 (SDL2)

## 1. Test GPL SDL2 (quick demo, green map, yellow sectors, blue units)
```bash
cd /home/rayray/conquerv5_clone/gpl-release/Src
gcc -o main_sdl2 main_sdl2.c -lSDL2 -lSDL2_ttf
./main_sdl2
```

## 2. Test Original SDL2 (same demo, original codebase)
```bash
cd /home/rayray/conquerv5_clone/original/Src/sdl2
gcc -o main_sdl2_original main_sdl2_original.c -lSDL2 -lSDL2_ttf
./main_sdl2_original
```

## 3. Full GPL SDL2 Build (replace curses, ready for sprites)
```bash
cd /home/rayray/conquerv5_clone/gpl-release/Src
make build
# or
make build_curses   # fallback to curses
```

## 4. Full Original SDL2 Build (replace curses, ready for sprites)
```bash
cd /home/rayray/conquerv5_clone/original/Src
make build
# or
make build_curses   # fallback to curses
```

## 5. Quick Test Script (Optional)
Save as `test_sdl2.sh` and run `bash test_sdl2.sh`:
```bash
#!/bin/bash
cd /home/rayray/conquerv5_clone/original/Src/sdl2
gcc -o main_sdl2_original main_sdl2_original.c -lSDL2 -lSDL2_ttf
./main_sdl2_original
```

## 6. Clean Build
```bash
make clean
```

## 7. Clobber (Full Clean)
```bash
make clobber
```

---

## Quick Start
1. **Test GPL SDL2:** `cd gpl-release/Src && gcc -o main_sdl2 main_sdl2.c -lSDL2 -lSDL2_ttf && ./main_sdl2`
2. **Test Original SDL2:** `cd original/Src/sdl2 && gcc -o main_sdl2_original main_sdl2_original.c -lSDL2 -lSDL2_ttf && ./main_sdl2_original`
3. **Build Full:** `cd original/Src && make build`

---

*Built with ❤️ and ☕ in Conquer V5 Modernization Phase 4a–4b.*
