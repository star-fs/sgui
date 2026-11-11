# sgui

Replacement IEM-style GUI objects for the Pure Data environment.

sgui is a **self-contained** pd external: it vendors the IEM-GUI helper
machinery (ported from pd-extended 0.44's `g_all_guis.c`) so it builds
against only the **public** `m_pd.h`. No pd internal headers
(`g_canvas.h`, `g_all_guis.h`, `s_stuff.h`, `t_tk.h`, ...) are needed, and
it runs on any 0.5x-generation pd — vanilla, pd-extended (Purr Data), or
PlugData.

## Objects

| name   | what it is                          | also usable as |
|--------|-------------------------------------|----------------|
| `sbng` | resizable "smart" bang              | `sbng`         |
| `svsl` | vertical slider                     | `svsl`, and **shadows pd's built-in `vsl`** |
| `shsl` | horizontal slider                   | `shsl`, and **shadows pd's built-in `hsl`** |
| `stgl` | toggle switch                       | `stgl`, and **shadows pd's built-in `tgl`** |
| `sknb` | rotary knob                         | `sknb`         |

### Shadowing

`svsl`/`shsl`/`stgl` register their class under the built-in names
`vsl`/`hsl`/`tgl` **and** keep their own `s`-prefixed name as an alias.
Because pd 0.5x registers object constructors in the `objectmaker` table,
registering a class under an already-taken name renames the old (built-in)
constructor to `<name>_aliased` and lets the new one win the name. The net
effect:

* In a patch, a plain `vsl`/`hsl`/`tgl` line uses the sgui object (once the
  matching sgui library has been loaded).
* The original built-in still works if you reference it explicitly as
  `vsl_aliased` / `hsl_aliased` / `tgl_aliased`.

The `s`-prefixed names always work and never collide.

## Requirements

* A C compiler (`cc`/`gcc`/`clang`).
* pd headers: a directory containing the public `m_pd.h`.
  On Linux distros that ship pd this is usually `/usr/include/pd` (or
  `/usr/local/include/pd`).

## Building

```sh
make
```

`make` auto-locates `m_pd.h` in this order:

1. `$PD_SRC` (env var or make variable: path to a pd **source tree**)
2. `/usr/include/pd`
3. `/usr/local/include/pd`
4. `./pd/src`

You can also point straight at the include directory: `make PDINC=/path/to/dir`
(where `m_pd.h` lives). If none of the above is found, `make` stops with a
clear error.

It builds **two kinds** of external:

* `sgui.pd_linux` — all five objects in one shared object. Load it
  explicitly with `pd -lib sgui` (or a `lib` message); its entry point is
  `sgui_setup`.
* `sbng.pd_linux`, `svsl.pd_linux`, `shsl.pd_linux`, `stgl.pd_linux`,
  `sknb.pd_linux` — one file per object name. pd loads these **on demand**
  when it meets the object in a patch (each entry point registers its own
  object). This is the usual way to use sgui: just drop the files on pd's
  search path and use the objects.

The on-demand files are the recommended installation, since they need no
explicit `-lib` and let pd load only what a patch actually uses.

On other platforms `make` picks the right suffix/flags automatically:
`pd_darwin` on macOS, `pd_win.dll` on MinGW.

## Installing

```sh
make install
```

copies the built externals and the `help/*.pd` files to `~/.pd/sgui/`.

## Usage

Add the sgui directory (or the individual `.pd_linux` files) to pd's path,
or install as above. Then use the objects in patches exactly as you would the
built-in widgets:

```
[svsl  15 100 0 127 0 0 empty empty empty 0 -9 0 10 -99865 -89565 -262144 0 1]
[shsl 130 15 0 127 0 0 empty empty empty -2 -8 0 10 -99865 -89565 -262144 50 0]
[stgl  50 20 0 empty empty stgl 0 -6 0 10 -100465 -258689 0 1 2.5]
[sbng  50 20 50 50 0 empty empty sbng 0 -6 0 10 -99865 -89565 -1]
[sknb  75 75 0 127 0 0 empty empty empty 0 -8 0 8 -99865 -89565 -262144 50 1]
```

Each object posts a one-line `sgui: <name> loaded` banner to the console when
its library is first loaded, which is a quick way to confirm sgui (and not a
built-in) is being used.

## Headless test

A patch instantiating all eight names (`sbng svsl shsl stgl sknb` plus the
shadowed `vsl hsl tgl`) can be smoke-tested without a display:

```sh
pd -nogui -stderr -batch -send "pd quit" sgui-test.pd
```

A correct build prints the five `sgui: ... loaded` banners and exits 0 with no
"couldn't create" / no errors. (The `-send "pd quit"` must come **before** the
patch file so pd has it queued before it opens the file.)

## Files

* `sgui.h` — private header: public-`m_pd.h` compat declarations + the vendored
  IEM-GUI API.
* `sgui_iem.c` — vendored IEM-GUI helper functions (pd-extended 0.44).
* `sgui.c` — all-in-one entry point (`sgui_setup`).
* `sbng.c` `svsl.c` `shsl.c` `stgl.c` `sknb.c` — the objects.
* `help/*.pd` — help/patch files.

## License

See `LICENSE.txt`. sgui is derived from Pure Data (Standard Improved BSD
License, Miller Puckette), the IEM-GUI objects (Thomas Musil, IEM KUG Graz),
and the sgui implementations (Star Morin).
