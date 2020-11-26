-- Copyright 2019-2020 Michael Fisher <mfisher@kushview.net>

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

local vector = require ('kv.vector')
local audio  = require ('kv.audio')
local midi   = require ('kv.midi')

local st = os.clock()
local completed, fails, checks = 0, 0, 0

local function bar (size)
   if type(size) ~= 'number' then size = 40 end
   size = math.tointeger (size)
   local out = ""
   for _ = 1,size do out = out.."-" end
   print (out)
end

local function expect (result, msg)
   if not result then
      fails = fails + 1
      print (msg or "failed!")
   else
      checks = checks + 1
   end
end

local function begin_test (name)
   name = tostring (name)
   print (string.format ("  %s", name))
end

local function test_db()
   expect (audio.togain (0.0) == 1.0)
   expect (audio.todecibels (1.0) == 0.0)
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

local function test_audiobuffer_vector()
   begin_test ("vector")
   local samplerate = 44100
   local nchans = 1
   local nframes = samplerate * 10
   local buf = audio.Buffer (nchans, nframes)
   print (string.format ("  %s", tostring (buf)))
   expect (buf:channels() == nchans)
   expect (buf:length() == nframes)
   local value = audio.round (math.random());

   local vec = nil
   for c = 1, nchans do
      vec = buf:vector (c)
      for f = 1, nframes do
         vec[f] = value
         expect (value == vec[f])
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

local function addnotes (b)
   local frame = 0
   for _ = 1, 1 do
      for note = 0, 127, 1 do
         b:insert (frame, midi.noteon (1, note, math.random (1, 127)))
         frame = frame + 10
         b:insert (frame, midi.noteoff (1, note, 0))
         frame = frame + 10
         b:insert (frame, midi.noteon (5, note, math.random (1, 127)))
         frame = frame + 10
         b:insert (frame, midi.noteoff (5, note, 0))
         frame = frame + 10
      end
   end
end

local function testnotes (b)
   local frame = 0
   local tick = 0
   local m = midi.Message()

   for f in b:events(m) do
      if tick % 2 == 0 then
         expect (m:isnoteon(), string.format("not note on: %s", tostring(m)))
      else
         expect (m:isnoteoff(), string.format("not note off: %s", tostring(m)))
      end

      if tick % 4 == 0 or tick % 4 == 1 then
         expect (m:channel() == 1)
      else
         expect (m:channel() == 5)
      end

      expect (frame == f)
      expect (m:channel(12) == 12)
      expect (m:channel() == 12)
      expect (m:channel(100) == 12)
      frame = frame + 10
      tick = tick + 1
   end

   expect (tick > 0 and frame > 0)
end

local function test_midibuffer_foreach()
   begin_test ("iterate")
   local b = midi.Buffer (1024 * 10)
   local a = midi.Buffer()

   addnotes (b)

   expect (a:capacity() == 0)
   a:swap (b);
   expect (a:capacity() > 0)
   local tframe = 0
   local tick = 0
   local m = midi.Message()
   for data, size, evframe in a:events() do
      expect (size == 3)
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

   begin_test ("swap/reserve")
   local c = midi.Buffer()
   c:swap (a)
   local oc = c:capacity()
   c:reserve (c:capacity() + 100)
   expect (c:capacity() == oc + 100)
   testnotes (c)

   begin_test ("clear")
   for _, buf in pairs({ a, b, c}) do buf:clear() end
end

local function test_midipipe()
   for _ = 1, 1 do
      local nbufs = 50
      local mp = midi.Pipe (nbufs)

      begin_test (tostring (mp))
      expect (mp[1] == nil, "__index[:int] should always be nil")

      begin_test ("__len")
      expect (#mp == nbufs)

      begin_test ("add notes")
      addnotes (mp:get(1))

      begin_test ("check notes")
      testnotes (mp:get(1))

      begin_test ("external swap")
      local mb = midi.Buffer()
      mb:swap (mp:get(1))
      testnotes (mb)

      for i = 1, #mp do mp:get(i):clear() end
      begin_test ("clear")
      mp:clear()
   end
end

local vecsize = 44100 * 10
local function test_vector()
   local vec = vector.new (vecsize)
   vec = nil; collectgarbage(); vec = vector.new (vecsize)
   begin_test (tostring (vec))

   expect (#vec == vecsize, string.format ("%d != %d", #vec, vecsize))
   for i, val in ipairs (vec) do
      expect (val == 0.0, string.format ("%f != %f", val, 0.0))
      vec[i] = 100.0
   end
   for _, val in ipairs (vec) do
      expect (val == 100.0, string.format ("%f != %f", val, 100.0))
   end
   vector.clear (vec)
   for _, val in ipairs (vec) do
      expect (val == 0.0, string.format ("%f != %f", val, 0.0))
   end
end

local function test_vector_clear()
   local vec = vector.new (vecsize)
   for i = 1, vecsize do vec[i] = 100.0 end
   for i = 1, vecsize do expect (vec[i] == 100.0) end
   for _ = 1, 1 do vector.clear (vec) end
   for i = 1, vecsize do expect (vec[i] == 0.0) end
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
      name = "audio.Vector",
      test = test_vector
   },
   {
      name = "audio.Vector (clear)",
      test = test_vector_clear
   },
   {
      name = "audio.Buffer: get/set/clear",
      test = test_audiobuffer_allocated
   },
   {
      name = "audio.Buffer: vector/clear",
      test = test_audiobuffer_vector
   },
   {
      name = "midi.Buffer (swap)",
      test = test_midibuffer_swap_table
   },
   {
      name = "midi.Buffer (foreach)",
      test = test_midibuffer_foreach
   },
   {
      name = "midi.Pipe",
      test = test_midipipe
   }
}

print ("Running tests....")
for i, t in ipairs (tests) do
   if (type(t.test) == 'function') then
      bar()
      print (string.format ("Testing: %s", tostring (t.name)))
      local x = os.clock()
      t.test()
      print(string.format("  elapsed time: %.3f (ms)", 1000.0 * (os.clock() - x)))
      print("")
      collectgarbage()
      completed = completed + 1
   end
end

bar()
print (string.format ("cases:     %d", completed))
print (string.format ("skipped:   %d", #tests - completed))
print (string.format ("passed:    %d", checks))
print (string.format ("failed:    %d", fails))
bar()
print (string.format ("Total time: %.3f (s)\n", os.clock() - st))
