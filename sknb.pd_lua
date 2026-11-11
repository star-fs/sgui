--[[
sgui - replacement IEM-style GUI objects for Pure Data

sknb.pd_lua - the sknb rotary knob as a pdlua GUI object.

PlugData cannot run the Tk drawing of the C sknb (it has no Tk), and it
only draws native widgets for class names it knows.  pdlua GUI objects,
however, are drawn natively by PlugData (and by vanilla pd with pdlua
installed), so this is sknb re-implemented on pdlua's paint API.

It takes the same creation arguments, messages and save format as the C
sknb (sknb.c), so patches move freely between the two:

  sknb <size> <size> <min> <max> <lin0_log1> <init> <snd> <rcv> <label>
       <ldx> <ldy> <fontstyle> <fontsize> <bcol> <fcol> <lcol> <val>
       [<steady>]

pd's native loader runs before pdlua's, so wherever sknb.pd_linux (or the
sgui library) provides sknb, that one wins; install this file *instead of*
sknb.pd_linux for PlugData (see "make install-plugdata").
]]

local sknb = pd.Class:new():register("sknb")

local DEFAULTSIZE = 32
local MINSIZE = 8     -- IEM_GUI_MINSIZE
local MAXSIZE = 1000  -- IEM_GUI_MAXSIZE
local SL_MINSIZE = 2  -- IEM_SL_MINSIZE

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

function sknb:initialize(sel, atoms)
    self.inlets = 1
    self.outlets = 1

    -- defaults, as in sknb_new()
    local w, h = DEFAULTSIZE, DEFAULTSIZE
    self.min, self.max = 0.0, 127.0
    self.lin0_log1 = 0
    self.isa = 0
    self.snd, self.rcv, self.lab = "empty", "empty", "empty"
    self.ldx, self.ldy = 0, -8
    self.fstyle, self.fontsize = 0, 8
    self.bcol, self.fcol, self.lcol = 0xfcfcfc, 0x000000, 0x000000
    self.val = 0
    self.steady = 1

    local n = #atoms
    if n == 17 or n == 18 then
        local function num(i) return tonumber(atoms[i]) or 0 end
        w, h = num(1), num(2)
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

    -- sknb quirk: the knob size is the *first* argument
    self.size = clip(math.floor(w), SL_MINSIZE, MAXSIZE)
    self.size = math.max(self.size, MINSIZE)
    self:check_minmax(self.min, self.max)
    self.val = clip(self.val, 0, self:maxval())
    self.pos = self.val

    self:set_size(self.size, self.size)
    return true
end

function sknb:postinitialize()
    self:bind_receive(self.rcv)
    if self.isa & 1 ~= 0 then
        -- stands in for iemgui's loadbang output
        self.initclock = pd.Clock:new():register(self, "loadbang")
        self.initclock:delay(0)
    end
end

function sknb:finalize()
    if self.receiver then self.receiver:destruct() end
    if self.initclock then self.initclock:destruct() end
end

function sknb:maxval()
    return 100 * self.size - 100
end

function sknb:check_k()
    if self.lin0_log1 ~= 0 then
        self.k = math.log(self.max / self.min) / (self.size - 1)
    else
        self.k = (self.max - self.min) / (self.size - 1)
    end
end

function sknb:check_minmax(min, max)
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

function sknb:bind_receive(rcv)
    if self.receiver then
        self.receiver:destruct()
        self.receiver = nil
    end
    if rcv ~= "empty" then
        self.receiver = pd.Receive:new():register(self,
            self:canvas_realizedollar(rcv), "receive")
    end
end

-- the knob's outputs
function sknb:value()
    local out
    if self.lin0_log1 ~= 0 then
        out = self.min * math.exp(self.k * self.val * 0.01)
    else
        out = self.val * 0.01 * self.k + self.min
    end
    if out < 1.0e-10 and out > -1.0e-10 then out = 0.0 end
    return out
end

function sknb:send_value()
    local out = self:value()
    self:outlet(1, "float", {out})
    if self.snd ~= "empty" then
        pd.send(self:canvas_realizedollar(self.snd), "float", {out})
    end
end

-- input on the receive name behaves like input on the inlet
function sknb:receive(sel, atoms)
    self:dispatch(1, sel, atoms)
end

-- like iemgui's x_put_in2out: no feedback loop when snd == rcv
function sknb:put_in2out()
    return not (self.snd ~= "empty" and self.snd == self.rcv)
end

function sknb:changed()
    self:repaint()
    self:save_args()
end

-- f -> val (pixels * 100), without redrawing
function sknb:store_value(f)
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

function sknb:set_value(f)
    self:store_value(f)
    self:changed()
end

function sknb:loadbang()
    self:repaint()
    self:send_value()
end

-- persist the current state as creation arguments (= sknb_save)
function sknb:save_args()
    self:set_args({
        self.size, self.size, self.min, self.max, self.lin0_log1, self.isa,
        self.snd, self.rcv, self.lab, self.ldx, self.ldy,
        self.fstyle, self.fontsize,
        col2sym(self.bcol), col2sym(self.fcol), col2sym(self.lcol),
        self.val, self.steady
    })
end

-- messages (the same set as the C sknb, except the canvas-editing
-- "delta"/"pos", which pdlua objects cannot do)

function sknb:in_1_bang()
    self:send_value()
end

function sknb:in_1_float(f)
    self:set_value(f)
    if self:put_in2out() then self:send_value() end
end

function sknb:in_1_list(atoms)
    local f = tonumber(atoms[1])
    if f then self:in_1_float(f) end
end

function sknb:in_1_set(atoms)
    self:set_value(tonumber(atoms[1]) or 0)
end

function sknb:in_1_size(atoms)
    local s = tonumber(atoms[1])
    if not s then return end
    -- keep the knob's value: val is in pixels, so rescale it
    local v = self:value()
    self.size = clip(math.floor(s), MINSIZE, MAXSIZE)
    self:check_k()
    self:set_size(self.size, self.size)
    self:set_value(v)
end

-- PlugData resizes a pdlua object (edit mode, dragging its edge) by
-- changing its size directly and repainting; pick the new size up here,
-- keeping the knob square and its value, so it is drawn and saved
function sknb:sync_size()
    local w, h = self:get_size()
    w, h = math.floor(w), math.floor(h)
    if w == self.size and h == self.size then return end
    local s = (w ~= self.size) and w or h
    s = clip(s, MINSIZE, MAXSIZE)
    local v = self:value()
    self.size = s
    self:check_k()
    if w ~= s or h ~= s then self:set_size(s, s) end
    self:store_value(v)
    self:save_args()
end

function sknb:in_1_range(atoms)
    self:check_minmax(tonumber(atoms[1]) or 0, tonumber(atoms[2]) or 0)
    self:save_args()
end

function sknb:in_1_log()
    self.lin0_log1 = 1
    self:check_minmax(self.min, self.max)
    self:save_args()
end

function sknb:in_1_lin()
    self.lin0_log1 = 0
    self:check_k()
    self:save_args()
end

function sknb:in_1_init(atoms)
    if (tonumber(atoms[1]) or 0) ~= 0 then
        self.isa = self.isa | 1
    else
        self.isa = self.isa & ~1
    end
    self:save_args()
end

function sknb:in_1_steady(atoms)
    self.steady = ((tonumber(atoms[1]) or 0) ~= 0) and 1 or 0
    self:save_args()
end

function sknb:in_1_color(atoms)
    local n = #atoms
    if n >= 1 then self.bcol = msgcolor(atoms[1], self.bcol) end
    if n >= 2 then self.fcol = msgcolor(atoms[2], self.fcol) end
    if n >= 3 then self.lcol = msgcolor(atoms[3], self.lcol) end
    self:changed()
end

function sknb:in_1_send(atoms)
    self.snd = name(atoms[1])
    self:save_args()
end

function sknb:in_1_receive(atoms)
    self.rcv = name(atoms[1])
    self:bind_receive(self.rcv)
    self:save_args()
end

function sknb:in_1_label(atoms)
    self.lab = name(atoms[1])
    self:repaint()
    self:save_args()
end

function sknb:in_1_label_pos(atoms)
    self.ldx = tonumber(atoms[1]) or self.ldx
    self.ldy = tonumber(atoms[2]) or self.ldy
    self:changed()
end

function sknb:in_1_label_font(atoms)
    self.fstyle = math.floor(tonumber(atoms[1]) or self.fstyle)
    self.fontsize = math.max(4, tonumber(atoms[2]) or self.fontsize)
    self:changed()
end

-- PlugData's properties panel: pdlua calls properties() to fill it in and
-- the prop_* method of each field with the edited value.  This needs a
-- pdlua with the properties API (PlugData newer than 0.9.3); older ones
-- never call it, and the messages below still do the same.

function sknb:properties(p)
    p:new_frame("Dimensions", 1)
    p:add_int("Size", "prop_size", self.size, MINSIZE, MAXSIZE)

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

function sknb:prop_size(v) self:in_1_size({v}) end
function sknb:prop_min(v) self:in_1_range({v, self.max}) end
function sknb:prop_max(v) self:in_1_range({self.min, v}) end
function sknb:prop_steady(v) self:in_1_steady({v}) end

function sknb:prop_log(v)
    if v ~= 0 then self:in_1_log() else self:in_1_lin() end
end

function sknb:prop_init(v) self:in_1_init({v}) end

function sknb:prop_bcol(c)
    self.bcol = propcolor(c, self.bcol)
    self:repaint()
    self:save_args()
end

function sknb:prop_fcol(c)
    self.fcol = propcolor(c, self.fcol)
    self:repaint()
    self:save_args()
end

function sknb:prop_lcol(c)
    self.lcol = propcolor(c, self.lcol)
    self:repaint()
    self:save_args()
end

function sknb:prop_label(s) self:in_1_label({s}) end
function sknb:prop_ldx(v) self:in_1_label_pos({v, self.ldy}) end
function sknb:prop_ldy(v) self:in_1_label_pos({self.ldx, v}) end
function sknb:prop_fontsize(v) self:in_1_label_font({self.fstyle, v}) end
function sknb:prop_send(s) self:in_1_send({s}) end
function sknb:prop_receive(s) self:in_1_receive({s}) end

-- mouse (= sknb_click / sknb_motion): vertical drag turns the knob,
-- 1 pixel per step; without "steady" a click jumps to the pointer height

function sknb:mouse_down(x, y)
    if self.steady == 0 then
        self.val = math.floor(100.0 * (self.size - y))
    end
    self.val = clip(self.val, 0, self:maxval())
    self.pos = self.val
    self.last_y = y
    self:repaint()
    self:send_value()
end

function sknb:mouse_drag(x, y)
    local old = self.val
    self.pos = self.pos - 100 * math.floor(y - self.last_y)
    self.last_y = y
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

function sknb:mouse_up(x, y)
    self:save_args()
end

-- drawing (= sknb_draw_new): a filled circle, and a pie slice starting at
-- 6 o'clock and growing clockwise with the value

local function rgb(c)
    return (c >> 16) & 0xff, (c >> 8) & 0xff, c & 0xff
end

function sknb:paint(g)
    self:sync_size()
    local s = self.size
    local r, gr, b = rgb(self.bcol)
    g:set_color(r, gr, b, 1)
    g:fill_ellipse(0, 0, s, s)

    -- Tk arc: -start -90 -extent -(val*360/(size*100) + 1) degrees
    local extent = self.val * 360 / (s * 100) + 1
    local cx, cy, rad = s / 2, s / 2, s / 2 - 1
    local steps = math.max(2, math.ceil(extent / 6))
    local p = Path(cx, cy)
    for i = 0, steps do
        -- Tk angles run counterclockwise from 3 o'clock with y up
        local a = math.rad(-90 - extent * i / steps)
        p:line_to(cx + rad * math.cos(a), cy - rad * math.sin(a))
    end
    p:close()
    r, gr, b = rgb(self.fcol)
    g:set_color(r, gr, b, 1)
    g:fill_path(p)

    g:set_color(0, 0, 0, 1)
    g:stroke_ellipse(0, 0, s, s, 1)

    if self.lab ~= "empty" then
        r, gr, b = rgb(self.lcol)
        g:set_color(r, gr, b, 1)
        g:draw_text(self:canvas_realizedollar(self.lab),
            self.ldx, self.ldy - self.fontsize / 2, 200, self.fontsize)
    end
end
