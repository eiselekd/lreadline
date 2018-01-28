loadfile("dbg.lua")

local lreadline = require("lreadline");

lreadline.log( "[+] start: " .. lreadline.version .."\n" )

function string.startswith(String, Start)
   return string.sub(String, 1, string.len(Start)) == Start
end

function make_completer(def)
   return function (text, state)
      lreadline.log( "'%s : state %d" %{text,state} )
      for i, d in ipairs(def) do
	 if string.startswith(d,text) then
	    if (state == 0) then
	       lreadline.log( "[>] %s\n" %{d} )
	       return d;
	    end
	    state = state-1;
	 end
	 return nil
      end
   end;
end

lreadline.set_completer(make_completer({"cat","dog","mouse"}));

while 1 do
   s = lreadline.input('>> ')
   if (s == nil) then
      break;
   else
      print("[" .. s .. "]");
   end
end
