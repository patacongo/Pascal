{ Same as 023-linklist.pas, but using long sequences of pointers.  Not a
  reasonable example, but a good test. }

PROGRAM linklist2;
TYPE
  nodeptr = ^node;
  node =
    RECORD
      flink   : nodeptr;
      payload : INTEGER
    END;

VAR
  head  : nodeptr;
  nodes : ARRAY[1..3] OF node;

BEGIN
  head                        := @nodes[1];
  head^.flink                 := @nodes[2];
  head^.payload               := 1;
  head^.flink^.flink          := @nodes[3];
  head^.flink^.payload        := 2;
  head^.flink^.flink^.flink   := NIL;
  head^.flink^.flink^.payload := 3;

  WRITELN('Node: ', head^.payload);
  WRITELN('Node: ', head^.flink^.payload);
  WRITELN('Node: ', head^.flink^.flink^.payload);
END.

