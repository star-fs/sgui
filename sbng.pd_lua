--[[
sgui - replacement IEM-style GUI objects for Pure Data

sbng.pd_lua - the sbng resizable bang as a pdlua GUI object.

PlugData cannot run the Tk drawing of the C sbng and has no native widget
for the name "sbng"; pdlua GUI objects are drawn natively by PlugData, so
this is sbng re-implemented on pdlua's paint API (see sknb.pd_lua).

It takes the same creation arguments, messages and save format as the C
sbng (sbng.c), so patches move freely between the two:

  sbng <width> <height> <hold> <break> <init> <snd> <rcv> <label>
       <ldx> <ldy> <fontstyle> <fontsize> <bcol> <fcol> <lcol>

pd's native loader runs before pdlua's, so install this file *instead of*
sbng.pd_linux for PlugData (see "make install-plugdata").
]]

local sbng = pd.Class:new():register("sbng")

local DEFAULTSIZE = 15        -- IEM_GUI_DEFAULTSIZE
local MINSIZE = 8             -- IEM_GUI_MINSIZE
local MAXSIZE = 1000          -- IEM_GUI_MAXSIZE
local DEFAULTHOLD = 250       -- IEM_BNG_DEFAULTHOLDFLASHTIME
local DEFAULTBREAK = 50       -- IEM_BNG_DEFAULTBREAKFLASHTIME
local MINHOLD = 50            -- IEM_BNG_MINHOLDFLASHTIME
local MINBREAK = 10           -- IEM_BNG_MINBREAKFLASHTIME

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

local function name(a)
    if type(a) == "number" then return string.format("%d", math.floor(a)) end
    if type(a) ~= "string" or a == "" then return "empty" end
    return a
end

function sbng:initialize(sel, atoms)
    self.inlets = 1
    self.outlets = 1

    -- defaults, as in sbng_new()
    self.w, self.h = DEFAULTSIZE, DEFAULTSIZE
    local hold, brk = DEFAULTHOLD, DEFAULTBREAK
    self.isa = 0
    self.snd, self.rcv, self.lab = "empty", "empty", "empty"
    self.ldx, self.ldy = 17, 7
    self.fstyle, self.fontsize = 0, 10
    self.bcol, self.fcol, self.lcol = 0xfcfcfc, 0x000000, 0x000000

    if #atoms == 15 then
        local function num(i) return tonumber(atoms[i]) or 0 end
        self.w, self.h = math.floor(num(1)), math.floor(num(2))
        hold, brk = num(3), num(4)
        self.isa = math.floor(num(5))
        self.snd, self.rcv, self.lab = name(atoms[6]), name(atoms[7]), name(atoms[8])
        self.ldx, self.ldy = num(9), num(10)
        self.fstyle, self.fontsize = math.floor(num(11)), math.max(4, num(12))
        self.bcol = loadcolor(atoms[13], self.bcol)
        self.fcol = loadcolor(atoms[14], self.fcol)
        self.lcol = loadcolor(atoms[15], self.lcol)
    end
    self.w = clip(self.w, 1, MAXSIZE)
    self.h = clip(self.h, 1, MAXSIZE)
    self:check_minmax(brk, hold)

    self.flashed = false
    self.locked = false
    self:set_size(self.w, self.h)
    return true
end

function sbng:postinitialize()
    self.clock_hld = pd.Clock:new():register(self, "tick_hld")
    self.clock_brk = pd.Clock:new():register(self, "tick_brk")
    self.clock_lck = pd.Clock:new():register(self, "tick_lck")
    self:bind_receive(self.rcv)
    if self.isa & 1 ~= 0 then
        -- stands in for iemgui's loadbang output
        self.initclock = pd.Clock:new():register(self, "loadbang")
        self.initclock:delay(0)
    end
end

function sbng:finalize()
    if self.receiver then self.receiver:destruct() end
    for _, c in ipairs({self.clock_hld, self.clock_brk, self.clock_lck,
                        self.initclock}) do
        if c then c:destruct() end
    end
end

function sbng:check_minmax(brk, hold)
    if brk > hold then brk, hold = hold, brk end
    self.flashtime_break = math.max(MINBREAK, math.floor(brk))
    self.flashtime_hold = math.max(MINHOLD, math.floor(hold))
end

function sbng:bind_receive(rcv)
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
function sbng:receive(sel, atoms)
    self:dispatch(1, sel, atoms)
end

-- like iemgui's x_put_in2out: no feedback loop when snd == rcv
function sbng:put_in2out()
    return not (self.snd ~= "empty" and self.snd == self.rcv)
end

-- flash (= sbng_set): a bang during a flash blanks it for the break time
function sbng:flash()
    if self.flashed then
        self.flashed = false
        self:repaint()
        self.clock_brk:delay(self.flashtime_break)
        self.flashed = true
    else
        self.flashed = true
        self:repaint()
    end
    self.clock_hld:delay(self.flashtime_hold)
end

function sbng:tick_hld()
    self.flashed = false
    self:repaint()
end

function sbng:tick_brk()
    self:repaint()
end

function sbng:tick_lck()
    self.locked = false
end

-- output; from the inlet (bout1) the send name is skipped when it would
-- feed back, from the mouse and other input (bout2) it is always used
function sbng:bout(always_send)
    local in2out = self:put_in2out()
    if not in2out then
        self.locked = true
        self.clock_lck:delay(2)
    end
    self:outlet(1, "bang", {})
    if self.snd ~= "empty" and (always_send or in2out) then
        pd.send(self:canvas_realizedollar(self.snd), "bang", {})
    end
end

function sbng:loadbang()
    self:flash()
    self:bout(true)
end

-- persist the current state as creation arguments (= sbng_save)
function sbng:save_args()
    self:set_args({
        self.w, self.h, self.flashtime_hold, self.flashtime_break, self.isa,
        self.snd, self.rcv, self.lab, self.ldx, self.ldy,
        self.fstyle, self.fontsize,
        col2sym(self.bcol), col2sym(self.fcol), col2sym(self.lcol)
    })
end

-- messages (the same set as the C sbng, except the canvas-editing
-- "delta"/"pos", which pdlua objects cannot do)

function sbng:in_1_bang()
    if not self.locked then
        self:flash()
        self:bout(false)
    end
end

-- anything else that has no method of its own bangs (= sbng_bang2)
function sbng:in_1(sel, atoms)
    if not self.locked then
        self:flash()
        self:bout(true)
    end
end

function sbng:in_1_set()
    self:flash()
end

function sbng:in_1_size(atoms)
    local s = tonumber(atoms[1])
    if not s then return end
    self.w = clip(math.floor(s), MINSIZE, MAXSIZE)
    self.h = self.w
    self:set_size(self.w, self.h)
    self:repaint()
    self:save_args()
end

function sbng:in_1_flashtime(atoms)
    self:check_minmax(tonumber(atoms[1]) or 0, tonumber(atoms[2]) or 0)
    self:save_args()
end

function sbng:in_1_init(atoms)
    if (tonumber(atoms[1]) or 0) ~= 0 then
        self.isa = self.isa | 1
    else
        self.isa = self.isa & ~1
    end
    self:save_args()
end

function sbng:in_1_color(atoms)
    local n = #atoms
    if n >= 1 then self.bcol = msgcolor(atoms[1], self.bcol) end
    if n >= 2 then self.fcol = msgcolor(atoms[2], self.fcol) end
    if n >= 3 then self.lcol = msgcolor(atoms[3], self.lcol) end
    self:repaint()
    self:save_args()
end

function sbng:in_1_send(atoms)
    self.snd = name(atoms[1])
    self:save_args()
end

function sbng:in_1_receive(atoms)
    self.rcv = name(atoms[1])
    self:bind_receive(self.rcv)
    self:save_args()
end

function sbng:in_1_label(atoms)
    self.lab = name(atoms[1])
    self:repaint()
    self:save_args()
end

function sbng:in_1_label_pos(atoms)
    self.ldx = tonumber(atoms[1]) or self.ldx
    self.ldy = tonumber(atoms[2]) or self.ldy
    self:repaint()
    self:save_args()
end

function sbng:in_1_label_font(atoms)
    self.fstyle = math.floor(tonumber(atoms[1]) or self.fstyle)
    self.fontsize = math.max(4, tonumber(atoms[2]) or self.fontsize)
    self:repaint()
    self:save_args()
end

-- mouse (= sbng_click): one bang per click

function sbng:mouse_down(x, y)
    self:flash()
    self:bout(true)
end

-- drawing (= sbng_draw_new): a rectangle, foreground color while flashed

local function rgb(c)
    return (c >> 16) & 0xff, (c >> 8) & 0xff, c & 0xff
end

function sbng:paint(g)
    local r, gr, b = rgb(self.flashed and self.fcol or self.bcol)
    g:set_color(r, gr, b, 1)
    g:fill_rect(0, 0, self.w, self.h)
    g:set_color(0, 0, 0, 1)
    g:stroke_rect(0, 0, self.w, self.h, 1)

    if self.lab ~= "empty" then
        r, gr, b = rgb(self.lcol)
        g:set_color(r, gr, b, 1)
        g:draw_text(self:canvas_realizedollar(self.lab),
            self.ldx, self.ldy - self.fontsize / 2, 200, self.fontsize)
    end
end
