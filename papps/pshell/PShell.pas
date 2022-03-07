PROGRAM PShell;

  USES
    StringUtils in 'StringUtils.pas'

  CONST
    MaxCmdTokens = 5;
    MaxLineSize  = 80;
    MaxTokenSize = 20;

  TYPE

  VAR
    CommandLine    : STRING[MaxLineSize];
    CommandTokens  : ARRAY[1..MaxCmdTokens] OF STRING[MaxTokenSize];
    NumTokens      : integer;
    DisplayIndex   : integer;
    Quit           : boolean

  PROCEDURE ShowUsage;
  BEGIN
    WRITELN('PShell Commands:');
    WRITELN('  Q[uit] - Exit');
  END;

  PROCEDURE ParseLine;
  VAR
    TokenPosition  : integer;
    Success        : boolean

  BEGIN
     TokenPosition := 1;
     NumTokens     := 0;
     Success       := TokenizeString(CommandLine, ' ',
                                     CommandTokens[NumTokens + 1],
                                     TokenPosition);

     WHILE Success AND (NumTokens < MaxCmdTokens) DO
     BEGIN
       NumTokens   := NumTokens + 1;
       Success     := TokenizeString(CommandLine, ' ',
                                     CommandTokens[NumTokens + 1],
                                     TokenPosition);
     END
  END;

  PROCEDURE ExecuteCommand;
  VAR
    FirstCh : char

  BEGIN
     FirstCh := UpperCase(CharAt(CommandTokens[1], 1));
     CASE FirstCh OF
       'Q' : Quit := true;
       ELSE ShowUsage
     END
  END;

  BEGIN
    Quit := false;

    REPEAT
      WRITE('psh> ');
      READLN(CommandLine);
      ParseLine;
      WRITELN(NumTokens, ' Tokens in Command:');
      FOR DisplayIndex := 1 TO NumTokens DO
        WRITELN(DisplayIndex, ' [', CommandTokens[DisplayIndex], ']');
      IF NumTokens > 0 THEN
        ExecuteCommand;
    UNTIL Quit
  END.
