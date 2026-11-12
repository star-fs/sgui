--[[
sgui - replacement IEM-style GUI objects for Pure Data

shsl.pd_lua - the shsl horizontal slider as a pdlua GUI object.

PlugData draws every object whose class is named "vsl"/"hsl" (which the C
svsl/shsl register, to shadow pd's sliders) with its own native slider: a
thin line marking the value.  sgui's slider is a solid bar in the
foreground color from the minimum to the current value.  pdlua GUI objects
are drawn natively by PlugData, so this is shsl re-implemented on pdlua's
paint API (see sknb.pd_lua).  svsl.pd_lua and shsl.pd_lua are the same
code except for the orientation settings at the top.

It takes the same creation arguments, messages and save format as the C
shsl (shsl.c), so patches move freely between the two:

  shsl <width> <height> <min> <max> <lin0_log1> <init> <snd> <rcv> <label>
       <ldx> <ldy> <fontstyle> <fontsize> <bcol> <fcol> <lcol> <val>
       [<steady>]

It registers only "shsl"; in PlugData "hsl" stays PlugData's own
slider.  pd's native loader runs before pdlua's, so install this file
*instead of* shsl.pd_linux for PlugData (see "make install-plugdata").
]]

local slider = pd.Class:new():register("shsl")

-- orientation: the value runs along the height (bottom to top) for svsl,
-- along the width (left to right) for shsl
local VERTICAL = false
local DEFAULT_W, DEFAULT_H = 128, 15
local DEFAULT_LDX, DEFAULT_LDY = -2, -8

local MINSIZE = 8             -- IEM_GUI_MINSIZE (thickness)
local MAXSIZE = 1000          -- IEM_GUI_MAXSIZE
local SL_MINSIZE = 2          -- IEM_SL_MINSIZE (length)

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

function slider:initialize(sel, atoms)
    self.inlets = 1
    self.outlets = 1

    -- defaults, as in shsl_new()
    local w, h = DEFAULT_W, DEFAULT_H
    self.min, self.max = 0.0, 127.0
    self.lin0_log1 = 0
    self.isa = 0
    self.snd, self.rcv, self.lab = "empty", "empty", "empty"
    self.ldx, self.ldy = DEFAULT_LDX, DEFAULT_LDY
    self.fstyle, self.fontsize = 0, 10
    self.bcol, self.fcol, self.lcol = 0xfcfcfc, 0x000000, 0x000000
    self.val = 0
    self.steady = 1

    local n = #atoms
    if n == 17 or n == 18 then
        local function num(i) return tonumber(atoms[i]) or 0 end
        w, h = math.floor(num(1)), math.floor(num(2))
        self.min, self.max = num(3), num(4)
        self.lin0_log1 = (num(5) ~= 0) and 1 or 0
        self.isa = math.floor(num(6))
        self.snd, self.rcv, self.lab = name(atoms[7]), name(atoms[8]), name(atoms[9])
        self.ldx, self.ldy = num(10), num(11)
        self.fstyle, self.fontsize = math.floor(num(12)), math.max(4, num(13))
        self.bcol = loadcolor(atoms[14], self.bcol)
        self.fcol = loadcolor(atoms[15], self.fcol)
        self.lcol = loadcolor(atoms[16], self.lcol)
        if self.isa & 1 ~= 0 then self.val = math.floor(num(17)) end
        if n == 18 then self.steady = (num(18) ~= 0) and 1 or 0 end
    end

    self:set_dims(w, h)
    self:check_minmax(self.min, self.max)
    self.val = clip(self.val, 0, self:maxval())
    self.pos = self.val

    self:set_size(self.w, self.h)
    return true
end

function slider:postinitialize()
    self:bind_receive(self.rcv)
    if self.isa & 1 ~= 0 then
        -- stands in for iemgui's loadbang output
        self.initclock = pd.Clock:new():register(self, "loadbang")
        self.initclock:delay(0)
    end
end

function slider:finalize()
    if self.receiver then self.receiver:destruct() end
    if self.initclock then self.initclock:destruct() end
end

-- the slider's length (along the value) and thickness
function slider:length()
    if VERTICAL then return self.h else return self.w end
end

-- clip width/height like the C object: thickness as an iemgui size,
-- length to at least IEM_SL_MINSIZE
function slider:set_dims(w, h)
    if VERTICAL then
        self.w = clip(math.floor(w), MINSIZE, MAXSIZE)
        self.h = clip(math.floor(h), SL_MINSIZE, MAXSIZE)
    else
        self.w = clip(math.floor(w), SL_MINSIZE, MAXSIZE)
        self.h = clip(math.floor(h), MINSIZE, MAXSIZE)
    end
end

function slider:maxval()
    return 100 * self:length() - 100
end

function slider:check_k()
    if self.lin0_log1 ~= 0 then
        self.k = math.log(self.max / self.min) / (self:length() - 1)
    else
        self.k = (self.max - self.min) / (self:length() - 1)
    end
end

function slider:check_minmax(min, max)
    if self.lin0_log1 ~= 0 then
        if min == 0.0 and max == 0.0 then max = 1.0 end
        if max > 0.0 then
            if min <= 0.0 then min = 0.01 * max end
        else
            if min > 0.0 then max = 0.01 * min end
        end
    end
    self.min, self.max = min, max
    self.reverse = min > max
    self:check_k()
end

function slider:bind_receive(rcv)
    if self.receiver then
        self.receiver:destruct()
        self.receiver = nil
    end
    if rcv ~= "empty" then
        self.receiver = pd.Receive:new():register(self,
            self:canvas_realizedollar(rcv), "receive")
    end
end

-- the slider's output
function slider:value()
    local out
    if self.lin0_log1 ~= 0 then
        out = self.min * math.exp(self.k * self.val * 0.01)
    else
        out = self.val * 0.01 * self.k + self.min
    end
    if out < 1.0e-10 and out > -1.0e-10 then out = 0.0 end
    return out
end

function slider:send_value()
    local out = self:value()
    self:outlet(1, "float", {out})
    if self.snd ~= "empty" then
        pd.send(self:canvas_realizedollar(self.snd), "float", {out})
    end
end

-- input on the receive name behaves like input on the inlet
function slider:receive(sel, atoms)
    self:dispatch(1, sel, atoms)
end

-- like iemgui's x_put_in2out: no feedback loop when snd == rcv
function slider:put_in2out()
    return not (self.snd ~= "empty" and self.snd == self.rcv)
end

-- f -> val (pixels * 100), without redrawing
function slider:store_value(f)
    if self.reverse then
        f = clip(f, self.max, self.min)
    else
        f = clip(f, self.min, self.max)
    end
    local g
    if self.lin0_log1 ~= 0 then
        g = math.log(f / self.min) / self.k
    else
        g = (f - self.min) / self.k
    end
    self.val = math.floor(100.0 * g + 0.49999)
    self.pos = self.val
end

function slider:set_value(f)
    self:store_value(f)
    self:repaint()
    self:save_args()
end

function slider:loadbang()
    self:repaint()
    self:send_value()
end

-- persist the current state as creation arguments (= shsl_save)
function slider:save_args()
    self:set_args({
        self.w, self.h, self.min, self.max, self.lin0_log1, self.isa,
        self.snd, self.rcv, self.lab, self.ldx, self.ldy,
        self.fstyle, self.fontsize,
        col2sym(self.bcol), col2sym(self.fcol), col2sym(self.lcol),
        self.val, self.steady
    })
end

-- resize, keeping the slider's value (val is in pixels, so rescale it)
function slider:resize(w, h)
    local v = self:value()
    self:set_dims(w, h)
    self:check_k()
    self:store_value(v)
end

-- messages (the same set as the C shsl, except the canvas-editing
-- "delta"/"pos", which pdlua objects cannot do)

function slider:in_1_bang()
    self:send_value()
end

function slider:in_1_float(f)
    self:set_value(f)
    if self:put_in2out() then self:send_value() end
end

function slider:in_1_list(atoms)
    local f = tonumber(atoms[1])
    if f then self:in_1_float(f) end
end

function slider:in_1_set(atoms)
    self:set_value(tonumber(atoms[1]) or 0)
end

-- "size <width> [<height>]"
function slider:in_1_size(atoms)
    local w = tonumber(atoms[1])
    if not w then return end
    self:resize(w, tonumber(atoms[2]) or self.h)
    self:set_size(self.w, self.h)
    self:repaint()
    self:save_args()
end

function slider:in_1_range(atoms)
    self:check_minmax(tonumber(atoms[1]) or 0, tonumber(atoms[2]) or 0)
    self:save_args()
end

function slider:in_1_log()
    self.lin0_log1 = 1
    self:check_minmax(self.min, self.max)
    self:save_args()
end

function slider:in_1_lin()
    self.lin0_log1 = 0
    self:check_k()
    self:save_args()
end

function slider:in_1_init(atoms)
    if (tonumber(atoms[1]) or 0) ~= 0 then
        self.isa = self.isa | 1
    else
        self.isa = self.isa & ~1
    end
    self:save_args()
end

function slider:in_1_steady(atoms)
    self.steady = ((tonumber(atoms[1]) or 0) ~= 0) and 1 or 0
    self:save_args()
end

function slider:in_1_color(atoms)
    local n = #atoms
    if n >= 1 then self.bcol = msgcolor(atoms[1], self.bcol) end
    if n >= 2 then self.fcol = msgcolor(atoms[2], self.fcol) end
    if n >= 3 then self.lcol = msgcolor(atoms[3], self.lcol) end
    self:repaint()
    self:save_args()
end

function slider:in_1_send(atoms)
    self.snd = name(atoms[1])
    self:save_args()
end

function slider:in_1_receive(atoms)
    self.rcv = name(atoms[1])
    self:bind_receive(self.rcv)
    self:save_args()
end

function slider:in_1_label(atoms)
    self.lab = name(atoms[1])
    self:repaint()
    self:save_args()
end

function slider:in_1_label_pos(atoms)
    self.ldx = tonumber(atoms[1]) or self.ldx
    self.ldy = tonumber(atoms[2]) or self.ldy
    self:repaint()
    self:save_args()
end

function slider:in_1_label_font(atoms)
    self.fstyle = math.floor(tonumber(atoms[1]) or self.fstyle)
    self.fontsize = math.max(4, tonumber(atoms[2]) or self.fontsize)
    self:repaint()
    self:save_args()
end

-- PlugData's properties panel: pdlua calls properties() to fill it in and
-- the prop_* method of each field with the edited value.  This needs a
-- pdlua with the properties API (PlugData newer than 0.9.3); older ones
-- never call it, and the messages below still do the same.

function slider:properties(p)
    p:new_frame("Dimensions", 1)
    p:add_int("Width", "prop_width", self.w, VERTICAL and MINSIZE or SL_MINSIZE, MAXSIZE)
    p:add_int("Height", "prop_height", self.h, VERTICAL and SL_MINSIZE or MINSIZE, MAXSIZE)

    p:new_frame("General", 1)
    p:add_float("Minimum", "prop_min", self.min)
    p:add_float("Maximum", "prop_max", self.max)
    p:add_check("Logarithmic", "prop_log", self.lin0_log1)
    p:add_check("Steady on Click", "prop_steady", self.steady)
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

function slider:prop_width(v) self:in_1_size({v, self.h}) end
function slider:prop_height(v) self:in_1_size({self.w, v}) end
function slider:prop_min(v) self:in_1_range({v, self.max}) end
function slider:prop_max(v) self:in_1_range({self.min, v}) end
function slider:prop_steady(v) self:in_1_steady({v}) end

function slider:prop_log(v)
    if v ~= 0 then self:in_1_log() else self:in_1_lin() end
end

function slider:prop_init(v) self:in_1_init({v}) end

function slider:prop_bcol(c)
    self.bcol = propcolor(c, self.bcol)
    self:repaint()
    self:save_args()
end

function slider:prop_fcol(c)
    self.fcol = propcolor(c, self.fcol)
    self:repaint()
    self:save_args()
end

function slider:prop_lcol(c)
    self.lcol = propcolor(c, self.lcol)
    self:repaint()
    self:save_args()
end

function slider:prop_label(s) self:in_1_label({s}) end
function slider:prop_ldx(v) self:in_1_label_pos({v, self.ldy}) end
function slider:prop_ldy(v) self:in_1_label_pos({self.ldx, v}) end
function slider:prop_fontsize(v) self:in_1_label_font({self.fstyle, v}) end
function slider:prop_send(s) self:in_1_send({s}) end
function slider:prop_receive(s) self:in_1_receive({s}) end

-- mouse (= shsl_click / shsl_motion): dragging along the slider moves
-- it 1 pixel per step; without "steady" a click jumps to the pointer

function slider:mouse_down(x, y)
    if self.steady == 0 then
        if VERTICAL then
            self.val = math.floor(100.0 * (self.h - y))
        else
            self.val = math.floor(100.0 * x)
        end
    end
    self.val = clip(self.val, 0, self:maxval())
    self.pos = self.val
    self.last_x, self.last_y = x, y
    self:repaint()
    self:send_value()
end

function slider:mouse_drag(x, y)
    local old = self.val
    if VERTICAL then
        self.pos = self.pos - 100 * math.floor(y - self.last_y)
    else
        self.pos = self.pos + 100 * math.floor(x - self.last_x)
    end
    self.last_x, self.last_y = x, y
    self.val = self.pos
    local maxval = self:maxval()
    if self.val > maxval then
        self.val = maxval
        self.pos = self.pos + 50
        self.pos = self.pos - self.pos % 100
    end
    if self.val < 0 then
        self.val = 0
        self.pos = self.pos - 50
        self.pos = self.pos - self.pos % 100
    end
    if old ~= self.val then
        self:repaint()
        self:send_value()
    end
end

function slider:mouse_up(x, y)
    self:save_args()
end

-- PlugData resizes a pdlua object (edit mode, dragging its edge) by
-- changing its size directly and repainting; pick the new size up here,
-- keeping the value, so it is drawn and saved
function slider:sync_size()
    local w, h = self:get_size()
    w, h = math.floor(w), math.floor(h)
    if w == self.w and h == self.h then return end
    self:resize(w, h)
    if self.w ~= w or self.h ~= h then self:set_size(self.w, self.h) end
    self:save_args()
end

-- drawing (= shsl_draw_new): the background box, and a solid bar in the
-- foreground color from the minimum end to the current value, inset BORDER
-- pixels so a border of background color shows around it.  The C object
-- insets 1: Tk centres the outline on the box edge, so the bar's first pixel
-- is already clear of it.  pdlua strokes the outline inside the box (pixel
-- 0), so the bar starts at 2 to leave the same 1-pixel background border.

local BORDER = 2

local function rgb(c)
    return (c >> 16) & 0xff, (c >> 8) & 0xff, c & 0xff
end

function slider:paint(g)
    self:sync_size()
    local w, h = self.w, self.h
    local r, gr, b = rgb(self.bcol)
    g:set_color(r, gr, b, 1)
    g:fill_rect(0, 0, w, h)
    g:set_color(0, 0, 0, 1)
    g:stroke_rect(0, 0, w, h, 1)

    -- the bar's extent, as the C object computes it (shsl_barcoords):
    -- val (0 .. (length-1)*100) scaled to the inner length
    local len = self:length()
    local inner = len - 2 * BORDER
    local p = 0
    if inner > 0 and len > 1 then
        p = math.floor(self.val * inner / (100.0 * (len - 1)) + 0.5)
    end
    local bx, by, bw, bh
    if VERTICAL then
        bx, by, bw, bh = BORDER, h - BORDER - p, w - 2 * BORDER, p
    else
        bx, by, bw, bh = BORDER, BORDER, p, h - 2 * BORDER
    end
    if bw > 0 and bh > 0 then
        r, gr, b = rgb(self.fcol)
        g:set_color(r, gr, b, 1)
        g:fill_rect(bx, by, bw, bh)
    end

    if self.lab ~= "empty" then
        r, gr, b = rgb(self.lcol)
        g:set_color(r, gr, b, 1)
        g:draw_text(self:canvas_realizedollar(self.lab),
            self.ldx, self.ldy - self.fontsize / 2, 200, self.fontsize)
    end
end
