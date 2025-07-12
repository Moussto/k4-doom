<h1 align="center">K4DOOM: DOOM Port on Kindle 4</h1>
<img width="1023" height="405" alt="Screenshot 2025-07-12 at 17 24 03" src="https://github.com/user-attachments/assets/058950a1-31b4-4b15-ba38-4ad568c11c5e" />

<p align="center">
  <a href="https://github.com/Moussto/k4-doom">
    <img src="https://img.shields.io/badge/Project-K4--DOOM-informational?style=for-the-badge&logo=doom&logoColor=white&color=E7352C" alt="K4 DOOM Project Badge" />
  </a>
  <a href="https://github.com/Moussto/talk-k4-doom-slides">
    <img src="https://img.shields.io/github/last-commit/Moussto/talk-k4-doom-slides?style=for-the-badge&color=brightgreen" alt="Last Commit Badge" />
  </a>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Docker-2496ED?style=for-the-badge&logo=docker&logoColor=white" alt="Docker Badge" />
  <img src="https://img.shields.io/badge/GCC-00599C?style=for-the-badge&logo=c&logoColor=white" alt="GCC Badge" />
  <img src="https://img.shields.io/badge/Linux-000000?style=for-the-badge&logo=linux&logoColor=white" alt="Linux Badge" />
  <img src="https://img.shields.io/badge/Kindle-FF9900?style=for-the-badge&logo=amazon&logoColor=white" alt="Kindle Badge" />
</p>

---

[demo.webm](https://github.com/user-attachments/assets/2d42f39d-f998-4af9-9106-26cf0ba7714e)


A Doom port (and toolchain) that is playable on a Kindle 4 

Toolchain (releases tab) can be used in a Linux container to compile the DOOM source code.

Magic is in `doomgeneric_kfb.c` (Kindle Framebuffer logic).

Multiple modes of display are implemented
- `greyscale` (Native 8 bit greyscale)
- `blackwhite` (Black and white 1 bit black/white)
- `dithered` (Dithered + Black and white 1 bit) (Default mode)


---

## 🧪 How-To

```shell
./doom -episode 1 --mode greyscale

./doom -episode 1 --mode blackwhite 

./doom -episode 1
```


