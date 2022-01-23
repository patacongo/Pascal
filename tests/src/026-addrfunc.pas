PROGRAM AddrFunc;
TYPE
  iptr = ^INTEGER;

VAR
  num   : INTEGER;
  ptr   : iptr;
  paddr : ^INTEGER;

BEGIN
  num := 3000;

  (* Take the address of var *)
  ptr := @num;

  (* Let us see the value and the adresses *)
  paddr := addr(ptr);

  WRITELN('Value of num = ', num );
  WRITELN('Value available at ptr^ = ', ptr^ );
  WRITELN('Address at ptr = ', paddr^);

  (* Exercise the exit() procedure too *)
  exit(42);
END.
