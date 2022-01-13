PROGRAM linklist;
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
  ptr   : nodeptr;

BEGIN
  head             := @nodes[1];
  nodes[1].flink   := @nodes[2];
  nodes[1].payload := 1;
  nodes[2].flink   := @nodes[3];
  nodes[2].payload := 2;
  nodes[3].flink   := NIL;
  nodes[3].payload := 3;

  ptr := head;
  WHILE ptr <> NIL DO
  BEGIN
    WRITELN('Visit node = ', ptr^.payload);
    ptr := ptr^.flink;
  END;
END.

