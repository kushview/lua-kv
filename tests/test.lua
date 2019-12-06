-- Copyright 2019 Michael Fisher <mfisher@kushview.net>

-- Permission to use, copy, modify, and/or distribute this software for any
-- purpose with or without fee is hereby granted, provided that the above
-- copyright notice and this permission notice appear in all copies.

-- THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
-- REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
-- FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
-- INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
-- LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
-- OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
-- PERFORMANCE OF THIS SOFTWARE.

local audio = require ("audio")
local midi  = require ("midi")

local function expect (result, msg)
   assert (result, msg or "failed!")
end

local function begin_test (name)
   name = tostring (name)
   print (string.format ("  %s", name))
end

local function test_db()
   expect (audio.db2gain (0.0) == 1.0)
   expect (audio.gain2db (1.0) == 0.0)
end

local function test_audiobuffer()
   begin_test ("defaults")
   local buf = audio.Buffer()
   print (string.format ("  %s", tostring (buf)))
   expect (type (buf) == 'userdata')
   expect (type (buf:raw (0)) == 'userdata')
   expect (type (buf:array()) == 'userdata')
   expect (buf:channels() == 0)
   expect (buf:length() == 0)
   expect (buf:cleared() == false)
   expect (buf:clear() == nil)
   expect (buf:get() == 0.0)
   expect (buf:get(0) == 0.0)
   expect (buf:get(0, 0) == 0.0)
end

local function test_audiobuffer_allocated()
   begin_test ("set/get")
   local samplerate = 44100
   local nchans = 1
   local nframes = samplerate * 10
   local buf = audio.Buffer (nchans, nframes)
   print (string.format ("  %s", tostring (buf)))
   expect (buf:channels() == nchans)
   expect (buf:length() == nframes)
   local value = audio.round (math.random());

   for c = 1, nchans do
      for f = 1, nframes do
         buf:set (c, f, value)
         expect (value == buf:get (c, f))
      end
   end

   begin_test ("clearing")
   expect (buf:cleared() == false)
   buf:clear()
   expect (buf:cleared() == true)
end

local function test_midibuffer_swap_table()
   begin_test ("swap")
   local buf1 = midi.Buffer (0)
   local buf2 = midi.Buffer (100)
   buf1:swap (buf2)
   expect (buf2:capacity() == 0)
   expect (buf1:capacity() == 100)
end

local function test_midibuffer_foreach()
   begin_test ("iterate")
   local b = midi.Buffer (1024 * 10)
   local a = midi.Buffer()
   local m = midi.Message()
   local frame = 0

   for _ = 1, 1 do
      for note = 0, 127, 1 do
         -- b:add (frame, 0x80, note, math.random (128) - 1)
         b:insert (frame, midi.noteon (1, note, math.random (0, 127)))
         frame = frame + 10
         b:insert (frame, midi.noteoff (1, note, 0))
         frame = frame + 10
         b:insert (frame, midi.noteon (5, note, math.random (0, 127)))
         frame = frame + 10
         b:insert (frame, midi.noteoff (5, note, 0))
         frame = frame + 10
      end
   end

   expect (a:capacity() == 0)
   a:swap (b);
   expect (a:capacity() > 0)
   local tframe = 0
   local tick = 0
   for data, size, evframe in a:iter() do
      m:update (data, size)

      if tick % 4 == 0 or tick % 4 == 1 then
         expect (m:channel() == 1)
      else
         expect (m:channel() == 5)
      end

      expect (tframe == evframe)
      expect (m:channel(12) == 12)
      expect (m:channel() == 12)
      tframe = tframe + 10
      tick = tick + 1
   end
   expect (tick > 0 and tframe > 0)
   b:clear()
   a:clear()
end

local tests = {
   {
      name = "audio dB conversions",
      test = test_db
   },
   {
      name = "audio.Buffer: empty",
      test = test_audiobuffer
   },
   {
      name = "audio.Buffer: allocated",
      test = test_audiobuffer_allocated
   },
   {
      name = "midi.Buffer (swap)",
      test = test_midibuffer_swap_table
   },
   {
      name = "midi.Buffer (foreach)",
      test = test_midibuffer_foreach
   }
}

for i, t in ipairs (tests) do
    if (type(t.test) == 'function') then
      print (string.format ("Testing: %s", tostring (t.name)))
      local x = os.clock()
      t.test()
      print(string.format("  elapsed time: %.3f (ms)", 1000.0 * (os.clock() - x)))
      if #tests ~= i then print("") end
   end
   collectgarbage()
end
