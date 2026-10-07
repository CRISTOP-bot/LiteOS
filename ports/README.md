# Ports

BusyBox, TCC, Lua y Nano están registrados como Git submodules con commits
fijados en el repositorio principal. Inicialízalos con:

```sh
git submodule update --init --recursive
```

**Todavía no están portados ni se incluyen en el sistema arrancable.** Cada
port se integrará solo cuando sus dependencias reales (libc, syscalls y
filesystem) estén disponibles, mediante configuración y patches pequeños,
sin copiar fuentes upstream ni fingir compatibilidad Linux.
