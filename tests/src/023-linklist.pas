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
  head            := @node[1];
  node[1].flink   := @node[2];
  node[1].payload := 1;
  node[2].flink   := @node[3];
  node[2].payload := 2;
  node[2].flink   := NIL;
  node[2].payload := 3;

  ptr := head;
  WHILE ptr <> NIL DO
  BEGIN
    WRITELN('Visit node = ', ptr^.payload);
    ptr := ptr^.flink;
  END;
END.

