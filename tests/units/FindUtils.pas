PROGRAM FindUtils;

USES
  FileUtils IN 'FileUtils.pas'

VAR
  SearchResult : TSearchRec;
  MatchPath : STRING[256];
  Success : BOOLEAN

BEGIN
  MatchPath := 'src/*.pex';
  Success   := FindFirst(MatchPath, 0, SearchResult);
  WHILE Success DO
  BEGIN
    WriteLn('- Name: ', SearchResult.name);
    WriteLn('  Attr: ', SearchResult.attr);
    WriteLn('  Time: ', SearchResult.time);
    WriteLn('  size: ', SearchResult.size);

    Success := FindNext(SearchResult)
  END;

  FindClose(SearchResult);
END.
