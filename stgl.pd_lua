--[[
sgui - replacement IEM-style GUI objects for Pure Data

stgl.pd_lua - the stgl toggle as a pdlua GUI object.

PlugData draws every object whose class is named "tgl" (which the C stgl
registers, to shadow pd's toggle) with its own square, X-marked toggle.
pdlua GUI objects are drawn natively by PlugData, so this is stgl
re-implemented on pdlua's paint API (see sknb.pd_lua): an outline box with
a rectangle inset in it, width and height independent.

It takes the same creation arguments, messages and save format as the C
stgl (stgl.c), so patches move freely between the two:

  stgl <width> <height> <init> <snd> <rcv> <label> <ldx> <ldy>
       <fontstyle> <fontsize> <bcol> <fcol> <lcol> <on> [<nonzero>]

It registers only "stgl"; in PlugData "tgl" stays PlugData's own toggle.
pd's native loader runs before pdlua's, so install this file *instead of*
stgl.pd_linux for PlugData (see "make install-plugdata").
]]

local stgl = pd.Class:new():register("stgl")

local DEFAULTSIZE = 15        -- IEM_GUI_DEFAULTSIZE
local MINSIZE = 8             -- IEM_GUI_MINSIZE
local MAXSIZE = 1000          -- IEM_GUI_MAXSIZE

-- iemgui_color_hex: the 30 preset colors
local PRESETS = {
    16579836, 10526880, 4210752, 16572640, 16572608,
    16579784, 14220504, 14220540, 14476540, 16308476,
    14737632, 8158332, 2105376, 16525352, 16559172,
    15263784, 1370132, 2684148, 3952892, 16003312,
    12369084, 6316128, 0, 9177096, 5779456,
    7874580, 2641940, 17488, 5256, 5767248
}

local function clip(v, lo, hi)
    if v < lo then return lo end
    if v > hi then return hi end
    return v
end

local function preset(i)
    return PRESETS[(math.floor(i) % #PRESETS) + 1]
end

-- "#rrggbb" symbol -> 0xrrggbb, or nil
local function hexcolor(a)
    if type(a) == "string" and a:sub(1, 1) == "#" then
        return (tonumber(a:sub(2), 16) or 0) & 0xffffff
    end
    return nil
end

-- a color as saved in a patch: "#rrggbb", preset index, or the -1-encoded
-- 18-bit RGB of pd < 0.51 (see iemgui_colfromatomload in sgui_iem.c)
local function loadcolor(a, dflt)
    local h = hexcolor(a)
    if h then return h end
    if type(a) == "string" then a = tonumber(a) end
    if type(a) ~= "number" then return dflt end
    a = math.floor(a)
    if a < 0 then
        a = -1 - a
        return ((a & 0x3f000) << 6) | ((a & 0xfc0) << 4) | ((a & 0x3f) << 2)
    end
    return preset(a)
end

-- a color in a "color" message: "#rrggbb", preset index, or -1-x raw RGB
-- (see iemgui_compatible_colorarg in sgui_iem.c)
local function msgcolor(a, dflt)
    local h = hexcolor(a)
    if h then return h end
    if type(a) ~= "number" then return dflt end
    a = math.floor(a)
    if a >= 0 then return preset(a) end
    return (-1 - a) & 0xffffff
end

local function col2sym(c)
    return string.format("#%06x", c & 0xffffff)
end

-- a color from PlugData's properties panel: {r, g, b} (0-255) or "#rrggbb"
local function propcolor(c, dflt)
    if type(c) == "table" then
        local function byte(i) return math.floor(tonumber(c[i]) or 0) & 0xff end
        return (byte(1) << 16) | (byte(2) << 8) | byte(3)
    end
    return hexcolor(c) or dflt
end

-- a name as the properties panel shows it: "empty" is blank
local function shown(s)
    return (s == "empty") and "" or s
end

local function name(a)
    if type(a) == "number" then return string.format("%d", math.floor(a)) end
    if type(a) ~= "string" or a == "" then return "empty" end
    return a
end

function stgl:initialize(sel, atoms)
    self.inlets = 1
    self.outlets = 1

    -- defaults, as in stgl_new()
    self.w, self.h = DEFAULTSIZE, DEFAULTSIZE
    self.isa = 0
    self.snd, self.rcv, self.lab = "empty", "empty", "empty"
    self.ldx, self.ldy = 17, 7
    self.fstyle, self.fontsize = 0, 10
    self.bcol, self.fcol, self.lcol = 0xfcfcfc, 0x000000, 0x000000
    local on, nonzero = 0, 1

    local n = #atoms
    if n == 14 or n == 15 then
        local function num(i) return tonumber(atoms[i]) or 0 end
        self.w, self.h = math.floor(num(1)), math.floor(num(2))
        self.isa = math.floor(num(3))
        self.snd, self.rcv, self.lab = name(atoms[4]), name(atoms[5]), name(atoms[6])
        self.ldx, self.ldy = num(7), num(8)
        self.fstyle, self.fontsize = math.floor(num(9)), math.max(4, num(10))
        self.bcol = loadcolor(atoms[11], self.bcol)
        self.fcol = loadcolor(atoms[12], self.fcol)
        self.lcol = loadcolor(atoms[13], self.lcol)
        on = num(14)
        if n == 15 then nonzero = num(15) end
    end
    self.w = clip(self.w, 1, MAXSIZE)
    self.h = clip(self.h, 1, MAXSIZE)
    self.nonzero = (nonzero ~= 0) and nonzero or 1
    if self.isa & 1 ~= 0 and on ~= 0 then
        self.on = self.nonzero
    else
        self.on = 0
    end

    self:set_size(self.w, self.h)
    return true
end

function stgl:postinitialize()
    self:bind_receive(self.rcv)
    if self.isa & 1 ~= 0 then
        -- stands in for iemgui's loadbang output
        self.initclock = pd.Clock:new():register(self, "loadbang")
        self.initclock:delay(0)
    end
end

function stgl:finalize()
    if self.receiver then self.receiver:destruct() end
    if self.initclock then self.initclock:destruct() end
end

function stgl:bind_receive(rcv)
    if self.receiver then
        self.receiver:destruct()
        self.receiver = nil
    end
    if rcv ~= "empty" then
        self.receiver = pd.Receive:new():register(self,
            self:canvas_realizedollar(rcv), "receive")
    end
end

-- input on the receive name behaves like input on the inlet
function stgl:receive(sel, atoms)
    self:dispatch(1, sel, atoms)
end

-- like iemgui's x_put_in2out: no feedback loop when snd == rcv
function stgl:put_in2out()
    return not (self.snd ~= "empty" and self.snd == self.rcv)
end

function stgl:output()
    self:outlet(1, "float", {self.on})
    if self.snd ~= "empty" then
        pd.send(self:canvas_realizedollar(self.snd), "float", {self.on})
    end
end

-- = stgl_set
function stgl:set(f)
    self.on = f
    if f ~= 0 then self.nonzero = f end
    self:repaint()
    self:save_args()
end

function stgl:loadbang()
    self:set(self.on)
    self:output()
end

-- persist the current state as creation arguments (= stgl_save)
function stgl:save_args()
    self:set_args({
        self.w, self.h, self.isa,
        self.snd, self.rcv, self.lab, self.ldx, self.ldy,
        self.fstyle, self.fontsize,
        col2sym(self.bcol), col2sym(self.fcol), col2sym(self.lcol),
        self.on, self.nonzero
    })
end

-- messages (the same set as the C stgl, except the canvas-editing
-- "delta"/"pos", which pdlua objects cannot do)

function stgl:in_1_bang()
    self.on = (self.on == 0) and self.nonzero or 0
    self:repaint()
    self:save_args()
    self:output()
end

function stgl:in_1_float(f)
    self:set(f)
    if self:put_in2out() then self:output() end
end

function stgl:in_1_list(atoms)
    local f = tonumber(atoms[1])
    if f then self:in_1_float(f) end
end

function stgl:in_1_set(atoms)
    self:set(tonumber(atoms[1]) or 0)
end

function stgl:in_1_nonzero(atoms)
    local f = tonumber(atoms[1]) or 0
    if f ~= 0 then self.nonzero = f end
    self:save_args()
end

-- "size w [h]": one argument makes it square
function stgl:in_1_size(atoms)
    local w = tonumber(atoms[1])
    if not w then return end
    self.w = clip(math.floor(w), MINSIZE, MAXSIZE)
    self.h = clip(math.floor(tonumber(atoms[2]) or self.w), MINSIZE, MAXSIZE)
    self:set_size(self.w, self.h)
    self:repaint()
    self:save_args()
end

function stgl:in_1_init(atoms)
    if (tonumber(atoms[1]) or 0) ~= 0 then
        self.isa = self.isa | 1
    else
        self.isa = self.isa & ~1
    end
    self:save_args()
end

function stgl:in_1_color(atoms)
    local n = #atoms
    if n >= 1 then self.bcol = msgcolor(atoms[1], self.bcol) end
    if n >= 2 then self.fcol = msgcolor(atoms[2], self.fcol) end
    if n >= 3 then self.lcol = msgcolor(atoms[3], self.lcol) end
    self:repaint()
    self:save_args()
end

function stgl:in_1_send(atoms)
    self.snd = name(atoms[1])
    self:save_args()
end

function stgl:in_1_receive(atoms)
    self.rcv = name(atoms[1])
    self:bind_receive(self.rcv)
    self:save_args()
end

function stgl:in_1_label(atoms)
    self.lab = name(atoms[1])
    self:repaint()
    self:save_args()
end

function stgl:in_1_label_pos(atoms)
    self.ldx = tonumber(atoms[1]) or self.ldx
    self.ldy = tonumber(atoms[2]) or self.ldy
    self:repaint()
    self:save_args()
end

function stgl:in_1_label_font(atoms)
    self.fstyle = math.floor(tonumber(atoms[1]) or self.fstyle)
    self.fontsize = math.max(4, tonumber(atoms[2]) or self.fontsize)
    self:repaint()
    self:save_args()
end

-- PlugData's properties panel: pdlua calls properties() to fill it in and
-- the prop_* method of each field with the edited value.  This needs a
-- pdlua with the properties API (PlugData newer than 0.9.3); older ones
-- never call it, and the messages below still do the same.

function stgl:properties(p)
    p:new_frame("Dimensions", 1)
    p:add_int("Width", "prop_width", self.w, MINSIZE, MAXSIZE)
    p:add_int("Height", "prop_height", self.h, MINSIZE, MAXSIZE)

    p:new_frame("General", 1)
    p:add_float("Nonzero Value", "prop_nonzero", self.nonzero)
    p:add_check("Init", "prop_init", self.isa & 1)

    p:new_frame("Colors", 1)
    p:add_color("Background", "prop_bcol", col2sym(self.bcol))
    p:add_color("Foreground", "prop_fcol", col2sym(self.fcol))
    p:add_color("Label", "prop_lcol", col2sym(self.lcol))

    p:new_frame("Label", 1)
    p:add_text("Text", "prop_label", shown(self.lab))
    p:add_int("X Offset", "prop_ldx", self.ldx, -MAXSIZE, MAXSIZE)
    p:add_int("Y Offset", "prop_ldy", self.ldy, -MAXSIZE, MAXSIZE)
    p:add_int("Font Size", "prop_fontsize", self.fontsize, 4, 500)

    p:new_frame("Communication", 1)
    p:add_text("Send Symbol", "prop_send", shown(self.snd))
    p:add_text("Receive Symbol", "prop_receive", shown(self.rcv))
end

function stgl:prop_width(v) self:in_1_size({v, self.h}) end
function stgl:prop_height(v) self:in_1_size({self.w, v}) end
function stgl:prop_nonzero(v) self:in_1_nonzero({v}) end

function stgl:prop_init(v) self:in_1_init({v}) end

function stgl:prop_bcol(c)
    self.bcol = propcolor(c, self.bcol)
    self:repaint()
    self:save_args()
end

function stgl:prop_fcol(c)
    self.fcol = propcolor(c, self.fcol)
    self:repaint()
    self:save_args()
end

function stgl:prop_lcol(c)
    self.lcol = propcolor(c, self.lcol)
    self:repaint()
    self:save_args()
end

function stgl:prop_label(s) self:in_1_label({s}) end
function stgl:prop_ldx(v) self:in_1_label_pos({v, self.ldy}) end
function stgl:prop_ldy(v) self:in_1_label_pos({self.ldx, v}) end
function stgl:prop_fontsize(v) self:in_1_label_font({self.fstyle, v}) end
function stgl:prop_send(s) self:in_1_send({s}) end
function stgl:prop_receive(s) self:in_1_receive({s}) end

-- mouse (= stgl_click): a click toggles

function stgl:mouse_down(x, y)
    self:in_1_bang()
end

-- PlugData resizes a pdlua object (edit mode, dragging its edge) by
-- changing its size directly and repainting; pick the new size up here so
-- it is drawn and saved
function stgl:sync_size()
    local w, h = self:get_size()
    w, h = math.floor(w), math.floor(h)
    if w ~= self.w or h ~= self.h then
        self.w = clip(w, MINSIZE, MAXSIZE)
        self.h = clip(h, MINSIZE, MAXSIZE)
        if self.w ~= w or self.h ~= h then self:set_size(self.w, self.h) end
        self:save_args()
    end
end

-- drawing (= stgl_draw_new): the outline box with a rectangle inset in it,
-- which takes the foreground color while on

local function rgb(c)
    return (c >> 16) & 0xff, (c >> 8) & 0xff, c & 0xff
end

function stgl:paint(g)
    self:sync_size()
    local w, h = self.w, self.h
    local r, gr, b = rgb(self.bcol)
    g:set_color(r, gr, b, 1)
    g:fill_rect(0, 0, w, h)
    g:set_color(0, 0, 0, 1)
    g:stroke_rect(0, 0, w, h, 1)

    r, gr, b = rgb((self.on ~= 0) and self.fcol or self.bcol)
    g:set_color(r, gr, b, 1)
    g:fill_rect(2, 2, w - 4, h - 4)
    g:set_color(0, 0, 0, 1)
    g:stroke_rect(2, 2, w - 4, h - 4, 1)

    if self.lab ~= "empty" then
        r, gr, b = rgb(self.lcol)
        g:set_color(r, gr, b, 1)
        g:draw_text(self:canvas_realizedollar(self.lab),
            self.ldx, self.ldy - self.fontsize / 2, 200, self.fontsize)
    end
end
