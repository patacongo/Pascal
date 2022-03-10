PROGRAM MatchName;
  USES
    FileUtils IN 'FileUtils.pas'

  VAR
    Result : boolean

BEGIN
  Result := MatchFileName('MyProgram.pex', '*.pex');
  IF Result THEN
    WRITELN('[MyProgram.pex] matches [*.pex]')
  ELSE
    WRITELN('[MyProgram.pex] does NOT match [*.pex]');

  Result := MatchFileName('Example1.dat', 'Example?.dat');
  IF Result THEN
    WRITELN('[Example1.dat matches [Example?.dat]')
  ELSE
    WRITELN('[Example1.dat] does NOT match [Example?.dat]');

  Result := MatchFileName('Document42.txt', 'Document*');
  IF Result THEN
    WRITELN('[Document42.txt matches [Document*]')
  ELSE
    WRITELN('[Document42.txt] does NOT match [Document*]');

  Result := MatchFileName('Document42.txt', '*42*');
  IF Result THEN
    WRITELN('[Document42.txt matches [*42*]')
  ELSE
    WRITELN('[Document42.txt] does NOT match [*42*]')
END.
