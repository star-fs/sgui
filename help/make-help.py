#!/usr/bin/env python3
"""Generate the sgui help patches (help/<object>-help.pd).

Each help patch holds an example instance and a panel of pd's own number,
symbol and toggle boxes that set its properties through messages, so it
works the same in vanilla pd (C objects) and PlugData (pdlua objects), which
has no properties dialog for the pdlua ones.  The panel reaches the example
through [r $0-sgui]; set it up, then copy the example into a patch: all its
properties are saved in its creation arguments.

Run from the repository root:  python3 help/make-help.py
"""

import os
import sys

# send/receive name prefix; tests pass a fixed one to reach the panel
PREFIX = sys.argv[1] if len(sys.argv) > 1 else "\\$0"
HERE = os.path.dirname(os.path.abspath(__file__))

ROW = 28            # row height of the panel
LABEL_X = 290       # panel: row labels
WIDGET_X = 440      # panel: first widget
FONT = 12


def esc(text):
    return text.replace(",", " \\,").replace(";", " \\;")


class Canvas:
    def __init__(self):
        self.lines = []
        self.n = 0

    def add(self, line):
        self.lines.append(line + ";")
        self.n += 1
        return self.n - 1

    def obj(self, x, y, text):
        return self.add("#X obj %d %d %s" % (x, y, text))

    def msg(self, x, y, text):
        return self.add("#X msg %d %d %s" % (x, y, text.replace("$", "\\$")))

    def text(self, x, y, text, width=None):
        return self.add("#X text %d %d %s" % (x, y, esc(text))
                        + ("" if width is None else ", f %d" % width))

    def connect(self, a, outlet, b, inlet):
        self.lines.append("#X connect %d %d %d %d;" % (a, outlet, b, inlet))


def name(key, i=None):
    return "%s-%s" % (PREFIX, key) + ("" if i is None else "-%d" % i)


def build(obj):
    main, logic = Canvas(), Canvas()

    # logic subpatch: fills in the panel's start values on load
    lb = logic.obj(20, 20, "loadbang")
    ly = [60]               # next free y in the logic subpatch

    def set_widget(rcv, value):
        y = ly[0]
        m = logic.msg(20, y, "set %s" % value)
        s = logic.obj(20, y + 25, "s %s" % rcv)
        logic.connect(lb, 0, m, 0)
        logic.connect(m, 0, s, 0)
        ly[0] += 55

    lx = [200]              # next free x for each row's message logic

    def to_example(src, outlet, x, y):
        s = logic.obj(x, y, "s %s" % name("sgui"))
        logic.connect(src, outlet, s, 0)

    def row_logic(key, kind, n, message, defaults):
        """[r] each widget, build the message, send it to the example"""
        x, y = lx[0], 20
        lx[0] += 170
        if n == 1:
            r = logic.obj(x, y, "r %s" % name(key, 1))
            m = logic.msg(x, y + 60, message)
            logic.connect(r, 0, m, 0)
            to_example(m, 0, x, y + 90)
            return
        # several values: the first inlet is hot, the others store their
        # value then bang the pack
        t = "s" if kind == "sym" else "f"
        p = logic.obj(x, y + 60, "pack %s" % " ".join(
            (str(d) if t == "f" else "s") for d in defaults))
        for i in range(n):
            r = logic.obj(x + 55 * i, y, "r %s" % name(key, i + 1))
            if t == "s":
                show = logic.msg(x + 55 * i, y + 180, "set $1")
                s = logic.obj(x + 55 * i, y + 210, "s %s-set" % name(key, i + 1))
                logic.connect(r, 0, show, 0)
                logic.connect(show, 0, s, 0)
            if i == 0:
                logic.connect(r, 0, p, 0)
            else:
                tr = logic.obj(x + 55 * i, y + 30, "t b %s" % t)
                logic.connect(r, 0, tr, 0)
                logic.connect(tr, 1, p, i)
                logic.connect(tr, 0, p, 0)
        m = logic.msg(x, y + 90, message)
        logic.connect(p, 0, m, 0)
        to_example(m, 0, x, y + 120)
        if t == "s":
            # a symbol pack has no start values: load them, which also
            # sends them, unchanged, to the example
            l = logic.msg(x, y + 150, "list %s" % " ".join(defaults))
            logic.connect(lb, 0, l, 0)
            logic.connect(l, 0, p, 0)

    # ---- front panel ----
    main.text(20, 10, obj["title"])
    for i, line in enumerate(obj["about"]):
        main.text(20, 32 + 18 * i, line)

    top = 32 + 18 * len(obj["about"]) + 45
    r = main.obj(30, top, "r %s" % name("sgui"))
    ex = main.obj(30, top + 30, obj["example"])
    main.connect(r, 0, ex, 0)
    out_y = top + 30 + obj["height"] + 25
    if obj["output"] == "float":
        out = main.add("#X floatatom 30 %d 8 0 0 0 - - -" % out_y)
    else:
        out = main.obj(30, out_y, "print %s" % obj["name"])
    main.connect(ex, 0, out, 0)
    main.text(30, out_y + 35, "set it up with the boxes on the right, then copy this %s "
              "into your patch: it keeps its settings." % obj["name"], 32)
    main.text(30, out_y + 110, "in pd-vanilla the object's own properties dialog "
              "(right-click > Properties) does all this too. the color pickers "
              "are ELSE's [colors] for PlugData (which bundles ELSE): without "
              "ELSE they can't be created - type #rrggbb colors instead.", 32)
    main.text(30, out_y + 240, "PlugData doesn't show the properties of pdlua "
              "objects (which these are in PlugData) in its inspector yet: "
              "configure the example here, then copy and paste it from this "
              "help patch into yours.", 32)

    y = top
    main.text(LABEL_X, y - 24, "properties (sent to the example as messages):")
    for label, kind, key, message, defaults in obj["rows"]:
        main.text(LABEL_X, y, label)
        x = WIDGET_X
        if kind == "bang":
            main.obj(x, y, "bng 15 250 50 0 %s empty empty 17 7 0 10 #fcfcfc #000000 #000000"
                     % name(key, 1))
            r = logic.obj(lx[0], 20, "r %s" % name(key, 1))
            to_example(r, 0, lx[0], 50)
            lx[0] += 170
        elif kind == "scale":
            main.obj(x, y, "tgl 15 0 %s %s empty 17 7 0 10 #fcfcfc #000000 #000000 0 1"
                     % (name(key, 1), name(key, 1) + "-set"))
            set_widget(name(key, 1) + "-set", defaults[0])
            r = logic.obj(lx[0], 20, "r %s" % name(key, 1))
            sel = logic.obj(lx[0], 50, "sel 0")
            lin = logic.msg(lx[0], 80, "lin")
            log = logic.msg(lx[0] + 50, 80, "log")
            logic.connect(r, 0, sel, 0)
            logic.connect(sel, 0, lin, 0)
            logic.connect(sel, 1, log, 0)
            to_example(lin, 0, lx[0], 110)
            to_example(log, 0, lx[0] + 50, 110)
            lx[0] += 170
        elif kind == "pick":
            # ELSE's [colors]: PlugData (which bundles ELSE) draws it as its
            # own color picker; a click opens it and the color comes out as
            # #rrggbb, as if typed into the color box above it
            sub = ["#N canvas 0 0 450 200 pick 0;"]
            for i in range(3):
                sub.append("#X obj %d 20 inlet;" % (20 + 140 * i))
                sub.append("#X obj %d 60 s %s;" % (20 + 140 * i, name(key, i + 1)))
            for i in range(3):
                sub.append("#X connect %d 0 %d 0;" % (2 * i, 2 * i + 1))
            # a subpatch is one object of the main canvas: its lines, then
            # the restore line that places it
            pd = main.add("\n".join(sub) + "\n#X restore %d %d pd pick;\n#X f 30" % (x, y + 30))
            for i in range(3):
                pk = main.obj(x + 87 * i, y, "else/colors")
                main.connect(pk, 0, pd, i)
            y += ROW
        elif kind == "tgl":
            main.obj(x, y, "tgl 15 0 %s %s empty 17 7 0 10 #fcfcfc #000000 #000000 0 1"
                     % (name(key, 1), name(key, 1) + "-set"))
            set_widget(name(key, 1) + "-set", defaults[0])
            row_logic(key, kind, 1, message, defaults)
        else:
            atom = "floatatom" if kind == "num" else "symbolatom"
            width = 6 if kind == "num" else 9
            for i, d in enumerate(defaults):
                main.add("#X %s %d %d %d 0 0 0 - %s %s"
                         % (atom, x, y, width, name(key, i + 1) + "-set", name(key, i + 1)))
                set_widget(name(key, i + 1) + "-set", d)
                x += 8 * width + 15
            row_logic(key, kind, len(defaults), message, defaults)
        y += ROW

    sub_y = max(y + 10, out_y + 330)
    main.text(LABEL_X, sub_y, "the messages are built in here:")
    width = max(lx[0] + 40, 600)
    height = max(ly[0] + 20, 220)
    sub = ["#N canvas 100 100 %d %d logic 0;" % (width, height)] + logic.lines
    sub.append("#X restore %d %d pd logic;" % (LABEL_X + 240, sub_y))

    head = "#N canvas 120 80 %d %d %d;" % (WIDGET_X + 290, sub_y + 50, FONT)
    # the subpatch is the last object of the main canvas
    return "\n".join([head] + main.lines + sub) + "\n"


def common(colors, ldx, ldy, fontsize):
    return [
        ("colors: bg fg label", "sym", "color", "color $1 $2 $3", colors),
        ("pick (PlugData)", "pick", "color", None, None),
        ("label", "sym", "label", "label $1", ["empty"]),
        ("label x y", "num", "lpos", "label_pos $1 $2", [ldx, ldy]),
        ("font, size", "num", "lfont", "label_font $1 $2", [0, fontsize]),
        ("send symbol", "sym", "send", "send $1", ["empty"]),
        ("receive symbol", "sym", "receive", "receive $1", ["empty"]),
    ]


COLORS = ["#fcfcfc", "#000000", "#000000"]

SLIDER_ROWS = [
    ("range (min max)", "num", "range", "range $1 $2", [0, 127]),
    ("logarithmic", "scale", "scale", None, [0]),
    ("steady on click", "tgl", "steady", "steady $1", [1]),
    ("init (keep value)", "tgl", "init", "init $1", [0]),
]

ABOUT_TAIL = [
    "everything here is also a message you can send it.",
    "an empty send/receive/label is the symbol empty.",
    "colors are #rrggbb.",
]

OBJECTS = [
    dict(name="sbng", title="sbng - a bang drawn as a rectangle in a box",
         about=["outputs a bang when clicked or sent anything."] + ABOUT_TAIL,
         example="sbng 15 15 250 50 0 empty empty empty 17 7 0 10 #fcfcfc #000000 #000000",
         height=15, output="print",
         rows=[("size (w h)", "num", "size", "size $1 $2", [15, 15]),
               ("flash (break hold)", "num", "flash", "flashtime $1 $2", [50, 250]),
               ("bang on load", "tgl", "init", "init $1", [0]),
               ("test", "bang", "test", None, None)]
              + common(COLORS, 17, 7, 10)),
    dict(name="stgl", title="stgl - a toggle drawn as a rectangle in a box",
         about=["outputs its nonzero value or 0 when clicked."] + ABOUT_TAIL,
         example="stgl 15 15 0 empty empty empty 17 7 0 10 #fcfcfc #000000 #000000 0 1",
         height=15, output="float",
         rows=[("size (w h)", "num", "size", "size $1 $2", [15, 15]),
               ("nonzero value", "num", "nonzero", "nonzero $1", [1]),
               ("init (keep state)", "tgl", "init", "init $1", [0])]
              + common(COLORS, 17, 7, 10)),
    dict(name="sknb", title="sknb - a knob",
         about=["drag up and down to turn it."] + ABOUT_TAIL,
         example="sknb 32 32 0 127 0 0 empty empty empty 0 -8 0 8 #fcfcfc #000000 #000000 0 1",
         height=32, output="float",
         rows=[("size", "num", "size", "size $1", [32])] + SLIDER_ROWS
              + common(COLORS, 0, -8, 8)),
    dict(name="svsl", title="svsl - a vertical slider drawn as a solid bar",
         about=["drag along it to set the value."] + ABOUT_TAIL,
         example="svsl 15 128 0 127 0 0 empty empty empty 0 -9 0 10 #fcfcfc #000000 #000000 0 1",
         height=128, output="float",
         rows=[("size (w h)", "num", "size", "size $1 $2", [15, 128])] + SLIDER_ROWS
              + common(COLORS, 0, -9, 10)),
    dict(name="shsl", title="shsl - a horizontal slider drawn as a solid bar",
         about=["drag along it to set the value."] + ABOUT_TAIL,
         example="shsl 128 15 0 127 0 0 empty empty empty -2 -8 0 10 #fcfcfc #000000 #000000 0 1",
         height=15, output="float",
         rows=[("size (w h)", "num", "size", "size $1 $2", [128, 15])] + SLIDER_ROWS
              + common(COLORS, -2, -8, 10)),
]

if __name__ == "__main__":
    outdir = sys.argv[2] if len(sys.argv) > 2 else HERE
    for o in OBJECTS:
        with open(os.path.join(outdir, "%s-help.pd" % o["name"]), "w") as f:
            f.write(build(o))
