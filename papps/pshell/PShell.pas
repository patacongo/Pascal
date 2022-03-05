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
    DisplayIndex   : integer

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

BEGIN
  CommandLine := 'This is a string';
  WRITELN('String: ', CommandLine);
  ParseLine;
  WRITELN('Tokens:');
  FOR DisplayIndex := 1 TO NumTokens DO
    WRITELN(CommandTokens[DisplayIndex])
END.