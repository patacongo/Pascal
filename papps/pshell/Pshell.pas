{
  Pshell.pas

    Copyright (C) 2022 Gregory Nutt. All rights reserved.
    Author: Gregory Nutt <gnutt@nuttx.org>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.
  3. Neither the name of the copyright holder nor the names of its
     contributors may be used to endorse or promote products derived from
     this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
  AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
}

PROGRAM PShell;

  USES
    StringUtils in 'StringUtils.pas';
    PshCommands in 'PshCommands.pas'

  CONST
    MaxCmdTokens = 5;
    MaxLineSize  = 80;
    MaxTokenSize = 20;

  TYPE

  VAR
    CommandLine       : STRING[MaxLineSize];
    CommandTokens     : ARRAY[1..MaxCmdTokens] OF STRING[MaxTokenSize];
    StringBufferAlloc : integer  = 1024;
    HeapAlloc         : integer  = 256;
    NumTokens         : integer;
    DisplayIndex      : integer;
    Wait              : boolean  = true;
    Quit              : boolean  = false

  PROCEDURE PshShowUsage;
  BEGIN
    WRITELN('PShell Commands:');
    WRITELN('  L[ist]');
    WRITELN('    Show Pascal Executables');
    WRITELN('  R[un] <PexFileName>');
    WRITELN('    Run a Pascal Executable');
    WRITELN('  D[bg] <PexFileName>');
    WRITELN('    Debug a Pascal Executable');
    WRITELN('  Q[uit]');
    WRITELN('    Exit');
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
       'L' : PshShowExecutables;
       'R' : PshRunExecutable(CommandTokens[2], StringBufferAlloc,
                              HeapAlloc, Wait, false);
       'D' : PshRunExecutable(CommandTokens[2], StringBufferAlloc,
                              HeapAlloc, Wait, true);
       'Q' : Quit := true;
       ELSE PshShowUsage
     END
  END;

  BEGIN
    (* Set the current working directory *)

    IF (Length(HomeDirectory) > 0) THEN
      ChDir(HomeDirectory);

    (* Then loop processing commands *)

    REPEAT
      (* Read the command line *)

      WRITE('psh> ');
      READLN(CommandLine);

      (* Parse the command line and process the command *)

      ParseLine;
      IF NumTokens > 0 THEN
        ExecuteCommand;
    UNTIL Quit
  END.
