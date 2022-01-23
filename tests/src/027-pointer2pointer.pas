PROGRAM Pointer2Pointer;
TYPE
  iptr = ^INTEGER;
  pointerptr = ^ iptr;

VAR
  num : INTEGER;
  ptr : iptr;
  pptr : pointerptr;
  x, y : ^INTEGER;

BEGIN
  num := 3000;

  (* Take the address of num *)
  ptr := @num;

  (* Take the address of ptr using address of operator @ *)
  pptr := @ptr;

  (* let us see the value and the adresses *)
  x := addr(ptr);
  y := addr(pptr);

  WRITELN('Value of num = ', num );
  WRITELN('Value available at ptr^ = ', ptr^ );
  WRITELN('Value available at pptr^^ = ', pptr^^);
  WRITELN('Address at ptr = ', x^); 
  WRITELN('Address at pptr = ', y^);
END.
