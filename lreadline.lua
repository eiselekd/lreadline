local lreadline = require"lreadline.core";

lreadline.set_completer = function (c)
   lreadline._set_completer(c);
end

lreadline.input = function (p)
   a,c = lreadline._input(p);
   if (c == 1) then
      print("Ctrl-c pressed:" .. c);
   end
   return a,c;
end

return lreadline;
