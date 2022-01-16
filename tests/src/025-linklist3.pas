{ Same as 023-linklist.pas except that it uses new() and dispose() }

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
  ptr   : nodeptr;
  curr  : nodeptr;
  next  : nodeptr;

BEGIN
  curr          := new(node);
  next          := new(node);

  head          := curr;
  curr^.flink   := next;
  curr^.payload := 1;

  curr          := next;
  next          := new(node);

  curr^.flink   := next;
  curr^.payload := 2;

  next^.flink   := NIL;
  next^.payload := 3;

  ptr := head;
  WHILE ptr <> NIL DO
  BEGIN
    WRITELN('Visit node = ', ptr^.payload);
    curr := ptr;
    ptr  := ptr^.flink;
    DISPOSE(curr);
  END;
END.

