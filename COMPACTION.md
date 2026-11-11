# COMPACTION — sgui → proper pd external (pd-extended / PlugData)

_Saved 2026-11-10; **updated 2026-11-11 — project complete & verified**._

**Objective**
Convert `~/git-repos/sgui` (Star Morin's 0.3x-era pd replacement GUI objects) into a proper, self-contained pd external that builds and runs on **pd-extended (Purr Data) and PlugData** (and vanilla pd 0.5x). No git commit unless asked.

**Important Details**

*Architecture (decided & verified):*
- Vendor the IEM-GUI machinery into the external so it needs **only the public `m_pd.h`** + a small compat declaration block. Verified the required prototypes and the `struct _widgetbehavior` body are **identical** across: pd 0.54.1 (`/usr/include/pd`), pure-data main (`/tmp/opencode/pure-data/src`), PlugData's pd fork (`/tmp/opencode/plugdata-pd/src`).
- Vendored iemgui = **pd-extended 0.44 variant** from `/tmp/opencode/pdext/src/g_all_guis.{h,c}` (zrnsm mirror) — exactly the API sgui calls (`iemgui_all_colfromload`, `iemgui_save(gui,srl,int*)`, …). Do NOT use modern pd's g_all_guis (`iemgui_all_loadcolors`, `x_private`, unsigned colors) — struct differs. `t_iemgui` layout is private to our objects → safe to vendor.
- Compat decls (in sgui.h): `IOWIDTH 7`, `IHEIGHT 3`, `t_glistmotionfn/keyfn`, widgetbehavior cb typedefs, `struct _widgetbehavior` (7 fn ptrs), `glist_getcanvas`, `glist_isvisible`, `glist_grab(glist,gobj,motionfn,keyfn,int,int)`, `text_xpix`, `text_ypix`, `canvas_fixlinesfor`, `canvas_deletelinesfor`, `canvas_realizedollar`, `canvas_dirty`, `EXTERN extern int sys_noloadbang`. All EXTERN (dllimport-correct on Windows; MinGW `-shared` exports `sgui_setup` automatically).
- Class-shadowing of built-in `vsl`/`hsl`/`tgl` is supported: pd's `class_addmethodtolist` renames the old objectmaker method to `<name>_aliased` (plugdata-pd m_class.c ~line 156-175).
- In pd 0.5x, `t_object` = `t_text`. Public m_pd.h already has: `sys_vgui`, `sys_queuegui/unqueuegui`, `gfxstub_new/deleteforkey`, `canvas_getcurrent`, `sys_font(_weight)`, `atom_string`, `binbuf_*`, `class_setwidget/savefn/propertiesfn`, `t_gpointer`, `s_bang/s_float`.

*Register (keep names):* sbng="sbng"; svsl: class **"vsl"** + creator "svsl"; shsl: class **"hsl"** + creator "shsl"; stgl: class **"tgl"** + creator "stgl"; sknb="sknb". helpsymbols sbng/svsl/shsl/stgl/sknb (help files exist in `help/`). `sgui_setup()` calls sbng/svsl/shsl/stgl/**sknb**_setup (original had typo `skng_setup`).

*Per-object facts (keep parsers/behavior verbatim):*
- Arg counts: sbng 15; svsl/shsl/sknb 17|18; stgl 14|15. Save format strings verified correct by `wc -c`: sbng `"ssiisiiiiisssiiiiiii"`=20; svsl/shsl/sknb `"ssiisiiffiisssiiiiiiiii"`=23; stgl `"ssiisiiisssiiiiiiiff"`=20.
- Methods: svsl/shsl/sknb: click(5f), motion(2f), dialog, loadbang, set, size, delta, pos, range, color, send, receive, label, label_pos, label_font, log, lin, init, steady. stgl: click, dialog, loadbang, set, size, delta, pos, color, send, receive, label, label_pos, label_font, init, nonzero.
- free(): pd_unbind rcv if rcv_able; gfxstub_deleteforkey; sbng also 3× clock_free; sknb also pd_unbind `iemgui_key_sym` if selected.
- sknb: `IEM_KNOB_DEFAULTSIZE 32`; knob square (size = first creation arg via `x_gui.x_h = clip(h); sknb_check_height(x, w)`); getrect uses `ypix-2`, `h+5`; `steady` default 1 (sliders too).
- Dialog strings: `pdtk_iemgui_dialog %%s |vsl|` / `|hsl|` / `|tgl|` / `|sbng|` / `KNOB` — keep verbatim.

*Original bugs to fix while rewriting:*
- `sgui.c`: `skng_setup()` → `sknb_setup()` (done).
- Duplicate `t_symbol *iemgui_key_sym=0;` in sbng.c+sknb.c → single def in `sgui_iem.c`, extern decl in `sgui.h` (done in both new files).
- `#if PD_MINOR_VERSION < 37` block in sknb.c → use `class_setpropertiesfn`/`class_setsavefn` unconditionally.
- Internal includes (config.h, m_pd.h, g_canvas.h, t_tk.h, g_all_guis.h, s_stuff.h/m_imp.h) → `#include "sgui.h"` only.
- Debug `post()`s in `sbng_new` (and `stgl_new`) → remove.
- sknb vgui `%x`-with-pointer → `%lx`.
- sknb_save `atom_getsymbol(binbuf_getvec(te_binbuf))` → `gensym("sknb")`.
- Redundant same-name `class_addcreator` for "sbng"/"sknb" → drop.
- (Done in sbng.c: also registered the previously-dead symbol/pointer/list/anything/flashtime methods — intended improvements.)

*Environment:*
- vanilla pd 0.54.1: `/usr/bin/pd`, headers `/usr/include/pd`. Purr Data 2.19.3: `/opt/purr-data` (installs no pd headers). DISPLAY=:1 (GUI smoke test possible later).
- Refs: `/tmp/opencode/pdext/src/g_all_guis.{h,c}` (vendor source — /tmp may be wiped; if gone, re-clone `https://github.com/zrnsm/pd-extended`), `/tmp/opencode/plugdata-pd`, `/tmp/opencode/pure-data`.

**Work State (as of 2026-11-11 — COMPLETE)**
- ✅ `sgui.h` — compat decls + vendored IEM header + `SGUI_EXPORT` + setup decls (complete).
- ✅ `sgui_iem.c` — full 0.44 helper port incl. `iemgui_key_sym` def + const fixes (complete).
- ✅ `sgui.c` — entry point, fixed setup calls (complete).
- ✅ `sbng.c`, `svsl.c`, `shsl.c`, `stgl.c`, `sknb.c` — all rewritten to the
  self-contained style (`#include "sgui.h"` only); each `xxx_setup` is
  `SGUI_EXPORT` and posts a one-line `sgui: <name> loaded` banner.
- ✅ `Makefile` — builds **all-in-one** `sgui.$(EXT)` **and** per-object
  `<name>.$(EXT)`; PDINC auto-detect; `-fvisibility=hidden`; install/clean.
- ✅ `README.md` — full build/usage/shadowing/test docs.
- ✅ `LICENSE.txt` — pd Standard Improved BSD + IEM (Thomas Musil) + Star Morin credits.
- ✅ Build: clean, zero warnings. Headless test: all 8 names create, correct
  order, EXIT 0, no errors/segfault. Shadowing of `vsl`/`hsl`/`tgl` **proven**
  at runtime (temporary `post()` probes in the constructors showed
  `creator=vsl/hsl/tgl` invoking our code, then reverted).

**Key findings added 2026-11-11**
- **Symbol interposition (the segfault):** vanilla pd 0.5x compiles its own
  `g_all_guis.c` into the binary and **exports** `iemgui_*` with a different
  `t_iemgui` layout. If our `.so` also exported those names, the PLT calls from
  our objects were interposed onto pd's built-in copies → crash in
  `sbng_new`/`iemgui_new_getnames`. **Fix:** `CFLAGS += -fvisibility=hidden`,
  and export only the `*_setup` entry points via `SGUI_EXPORT`
  (`__attribute__((visibility("default")))`) declared in `sgui.h`.
  Verify: `nm -D sbng.pd_linux` shows only `sbng_setup` exported.
- **On-demand per-object loading:** pd 0.5x `s_loader.c` `sys_do_load_lib`
  dlopens `<name>.$(EXT)` (or `<name>/<name>.$(EXT)`) the first time it meets
  object `<name>` and calls `<name>_setup`. No directory pre-scan. So the
  Makefile emits one small `.pd_linux` per object (own `.o` + `sgui_iem.o`)
  **and** the all-in-one `sgui.$(EXT)` (for `pd -lib sgui`).
- **Shadowing mechanism (confirmed from vanilla source):** builtin `vsl` is a
  *creator* on the objectmaker (g_slider.c:748), `hsl` a class (g_slider.c:746),
  `tgl` a class (g_toggle.c:375). Our `class_new(gensym("vsl"/"hsl"/"tgl"))`
  collides in `class_addmethodtolist` (m_class.c ~148-174): the **old** entry is
  renamed `<name>_aliased` and our constructor wins the name. In test patches
  the `s`-prefixed objects must precede the shadowed names so the shadowing
  class registers first.
- **Headless quit:** this pd 0.54.1 `-batch` never self-exits (runs until a
  "quit" msg to the "pd" object → `glob_quit`→`exit`). Use
  `pd -nogui -stderr -batch -send "pd quit" <file.pd>` — `-send` **before** the
  patch file (after it, pd treats it as a filename). An in-patch
  `loadbang→[pd quit]` message box does *not* quit (message_bang passes the
  responder as the initial binbuf_eval target; the canvas swallows the list).

**GUI smoke test (2026-11-11, DISPLAY=:1, pixel-analyzed — model can't view images)**
Test patch `/tmp/opencode/gui-test/gui-test.pd` (all 8 objects, mid-range init
values, labels) + per-object `.pd_linux`. Verified by measuring non-canvas
"ink" per widget region (a render = substantial ink in the widget's box).
- **vanilla pd 0.54.1 (`/usr/bin/pd`): FULL PASS.** All 5 libs load, no
  crash, no errors. Every widget rendered: sbng 44.9%, svsl 40.1%, shsl 41.1%,
  stgl 38.1%, sknb 53.6%, vsl 40.1%, hsl 41.1%, tgl 38.1% ink; column profile
  shows svsl/vsl dense vertical fills (141%), shsl/hsl 44% bands, sknb circular
  knob. (Window capture has a 33px dark title bar → image_y = canvas_y + 34.)
- **Purr Data 2.19.3 (`/opt/purr-data/bin/pd-l2ork`, pd-0.48-based): C-side OK,
  GUI does NOT render.** No crash; all 5 libs load; correct draw commands are
  generated (log shows `create rectangle … -tags …VSL -fill #54dc70`). But its
  JS GUI (`pdgui.js`) `eval()`s the `sys_vgui` Tcl string `.x%lx.c create …` as
  JavaScript → `SyntaxError: Unexpected token .` (leading `.`), so nothing draws.
  Built-in widgets render fine in the same Purr Data (they use a GUI-native draw
  path, not `sys_vgui`). ⇒ Purr Data GUI limitation for `sys_vgui`-drawing externals.
- **PlugData (`/usr/bin/plugdata`): SEGFAULT.** Crashes in **PlugData's own**
  `iemgui_all_sym2dollararg` (gdb: whole call chain in the main binary; our `.so`
  exports 0 `iemgui_*`, so not interposition). Triggered with s-only objects too
  (not just shadowed). Cause: PlugData's GUI treats objects with the IEM widget
  behavior as built-in IEM widgets and calls its internal `iemgui_save`/
  `iemgui_properties` on them, reading our **0.44-vendored** `t_iemgui` with
  PlugData's layout → wrong fields → crash.

**Root cause across forks:** sgui vendors the IEM machinery (0.44 `t_iemgui`
layout + classic `sys_vgui` ".x%lx.c create …" drawing). Vanilla pd's GUI is
generic (calls the object's own draw fn; never inspects the IEM struct) → works.
The pd-extended forks have *integrated* GUIs that special-case IEM widgets and
assume the fork's own `t_iemgui` layout / draw path → misrender (Purr Data) or
crash (PlugData). Self-containment (vendoring) and fork-GUI compatibility are in
tension.

**Next Move**
- Vanilla pd 0.5x target: **done & verified** (headless + GUI).
- Fork (Purr Data / PlugData) rendering is **NOT working** — needs a decision:
  (a) accept sgui as a vanilla-pd external (document the fork limitation in
  README); (b) attempt fork-compatible drawing (match each fork's `t_iemgui`
  layout + its GUI draw path — fork-specific, likely breaks vanilla, high effort);
  (c) investigate whether the forks expose a supported external draw API.
  Awaiting user direction. Git commit only if asked.

**Relevant Files**
- `/home/dad/git-repos/sgui/`: `sgui.h`, `sgui_iem.c`, `sgui.c`, `sbng.c`, `svsl.c`, `shsl.c`, `stgl.c`, `sknb.c` (all done); `Makefile`, `README.md`, `LICENSE.txt` (done); `help/*.pd` (keep); git repo, do not commit unless asked.
- Test dir `/tmp/opencode/pdtest/`: `sgui-test.pd` (all 8 names, s-prefixed first) + copies of the 6 built `.pd_linux`.

**UPDATE 2026-11-11 (later) — PlugData FIXED**
- Root cause confirmed: PlugData's `ObjectBase.cpp` picks a native GUI by
  *class name* (`vsl`/`hsl`→SliderObject, `tgl`→ToggleObject, `bng`, `knob`…),
  casts to modern `t_slider`/`t_toggle`, and calls the HOST's `iemgui_send/
  receive/label/all_sym2dollararg`, which deref `x_private->p_binbuf_valid`.
- Fix: `t_iemgui` now has `t_iemgui_private *x_private` after `x_w` (block laid
  out like pd's private struct: prevX/Y, 7 draw fn ptrs, binbuf_valid), allocated
  zeroed by `sgui_iemgui_init()` in every `_new` and freed by `sgui_iemgui_free()`.
  svsl/shsl gained `t_float x_fval; t_iem_orientation x_orientation` (set
  vertical/horizontal); stgl uses `t_float`. Offsets verified identical to
  plugdata-pd headers (offset-diff program). `iemgui_fixnames()` maps
  NULL/"" names (host send/receive set them) back to "empty".
- Verified: vanilla headless EXIT 0; PlugData 0.9.2 no crash, svsl/shsl/stgl/vsl/
  hsl/tgl render natively, drag outputs values, inspector works. sbng/sknb show
  as plain text boxes (no native PlugData widget for those names).
- Open option: register sbng's class as `bng` (shadowing) to get PlugData's
  native bang — not done; user decision.

**UPDATE 2026-11-11 — colour handling modernized**
- Bugs: properties dialog got decimal ints (pd>=0.51 Tk needs "#rrggbb" → Tk
  error); dialog reply/`color` msg "#rrggbb" symbols parsed as 0; creation args
  with "#hex" colours failed IS_A_FLOAT check → whole arg list dropped (hit every
  vanilla/PlugData-saved vsl/hsl/tgl loaded via shadowing).
- Fix (sgui_iem.c): iemgui_getcolorarg / iemgui_compatible_colorarg /
  iemgui_colfromatomload (18-bit legacy decode) / iemgui_all_loadcolors /
  iemgui_col2symbol; `color` follows modern semantics (pd_compatibilitylevel);
  iemgui_save now returns colour *symbols* (save fmt colours "sss"); dialogs
  send "#%06x". Objects use IS_A_COLOR for colour args + `colargs`.
- Verified: headless msgs + save; legacy ints decode like vanilla; GUI dialog
  Apply/OK/save in pd 0.54.1 (no Tcl errors); PlugData renders hex colours.

**UPDATE 2026-11-11 — sknb in PlugData via pdlua (user chose this route)**
- `sknb.pd_lua`: pdlua GUI port of sknb (same args/messages/save format; colours
  hex/int; set_args persists state; Receive for rcv). Verified in PlugData 0.9.2:
  renders, receive works, click/drag output correct, ctrl+s saves value/colours.
- `sgui.c`: under PlugData (dlsym RTLD_DEFAULT "plugdata_forward_message")
  sgui_setup skips sknb_setup so the Lua class can load (native loader wins
  otherwise). Vanilla unchanged (C sknb).
- Makefile `install-plugdata` → ~/Documents/plugdata/Externals/sgui without
  sknb.pd_linux. Known limits: label clipped, no props dialog/delta/pos, no
  shift fine-drag, init outputs via clock 0 not loadbang.

**UPDATE 2026-11-11 — full verification, sbng.pd_lua, install, commit**
- sbng.pd_lua added (same args/messages/save; flash hold/break clocks; lock).
  sgui_setup skips C sbng too under PlugData; install-plugdata omits
  sbng/sknb .pd_linux.
- Fixed: loadbang fired twice in pd>=0.47 (phased loadbang with LB_* arg) →
  all loadbang methods A_DEFFLOAT, act only on LB_LOAD. sknb `size`/dialog
  now keep the value (k was stale). Dialog headers: "Bang", "Knob" (Size only,
  hidden height via {}), labels with Tcl braces.
- Verified headless vanilla (all messages/outputs/save), GUI vanilla dialogs,
  PlugData all 5 objects (render, receive msgs, color/size, clicks, drag).
  xdotool clicks double-fire in PlugData even for built-in bng/tgl (artifact).
- Installed to ~/Documents/plugdata/Externals/sgui; added that dir to
  PlugData .settings <Paths> (backup .settings.bak-sgui-20261111).
