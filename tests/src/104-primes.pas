{ generate the primes between 3..10000 using a
  sieve containing odd integers in this range. }

program primes(output);

const
  wdlength = 59; {implementation dependent}
  maxbit = 58;
  w = 84; {w=n div wdlength div 2}

var
  sieve, primes : array[0..w] of set of 0..maxbit;
  next : record
           wordIndex, bitNumber : integer
         end;
  j, k, t, c : integer; empty:boolean;

begin {initialize}
  for t:=0 to w do
    begin
      sieve[t]  := [0..maxbit];
      primes[t] := []
    end;

  sieve[0]       := sieve[0] - [0];
  next.wordIndex := 0;
  next.bitNumber := 1;
  empty          := false;

  with next do
  repeat {find next prime}
    while not (bitNumber in sieve[wordIndex]) do
      bitNumber := succ(bitNumber);

    primes[wordIndex] := primes[wordIndex] + [bitNumber];
    c                 := 2 * bitNumber + 1;
    j                 := bitNumber;
    k                 := wordIndex;

    while k <= w do {eliminate}
    begin sieve[k] := sieve[k] - [j];
      k := k + wordIndex * 2;
      j := j + c;
      while j > maxbit do
        begin
          k := k + 1;
          j := j - wdlength
        end
    end;

    if sieve[wordIndex] = [] then
      begin
        empty     := true;
        bitNumber := 0
      end;

    while empty and (wordIndex < w) do
      begin
        wordIndex := wordIndex + 1;
        empty     := sieve[wordIndex] = []
      end
  until empty; {ends with}
end.
