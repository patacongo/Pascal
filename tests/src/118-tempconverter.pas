PROGRAM A_Basic_Program;
{****************************************}
{* Programmer: Bert G. Wachsmuth        *}
{* Date:       Sept. 28, 1995           *}
{*                                      *}
{* This program converts degree Celcius *}
{* to and from degree Fahrenheit. The   *}
{* user can choose the conversion from  *}
{* a menu.                              *} 
{****************************************}

VAR
   UserChoice : CHAR; { menu choice }
   UserInput  : REAL; { number to convert}
   Answer     : REAL; { converted answer }

{****************************************}
PROCEDURE ShowTheMenu;
{ This procedure shows the available     }
{ options to the user of the program.    }
BEGIN
     WRITELN;
     WRITELN('     A Basic Program');
     WRITELN('     ---------------');
     WRITELN(' a) Celcius to Fahrenheit');
     WRITELN(' b) Fahrenheit to Celcius');
     WRITELN;
     WRITELN(' x) To exit the program');
     WRITELN;
END;

{****************************************}
PROCEDURE GetUserChoice;
{ This procedure asks the user for their }
{ choice and stores the result in        }
{ UserMenuChoice.      }
BEGIN
    WRITE('Enter your choice:       ');
    READLN(UserChoice);
END;

{****************************************}
PROCEDURE GetNumberToConvert;
{ Asks the user for the number to be     }
{ converted                              }
BEGIN
   WRITE('Enter number to convert: ');
   READLN(UserInput);
END;

{****************************************}
PROCEDURE Wait;
{ Holds execution until user presses     }
{ RETURN                                 }
BEGIN
    WRITE('Press RETURN to continue ...');
    READLN;
END;

{****************************************}
FUNCTION ToFahrenheit(x: REAL): REAL;
{ Function to convert degrees Celcius to }
{ degree Fahrenheit.                     }
BEGIN
     ToFahrenheit := 9/5 * x + 32;
END;

{****************************************}
FUNCTION ToCelcius(x: REAL): REAL;
{ Function to convert degrees Fahrenheit }
{ to degree Celcius.                     }
BEGIN
     ToCelcius := 5/9 * (x - 32);
END;

{****************************************}
PROCEDURE DoTheConversion;
{ This procedure converts the number     }
{ contained in UserInput to the degree   }
{ according to the user's menu choice.   }
BEGIN
     IF (UserChoice = 'a') THEN
        answer := ToFahrenheit(UserInput);
     IF (UserChoice = 'b') THEN
        answer := ToCelcius(UserInput);
END;

{****************************************}
PROCEDURE DisplayTheAnswer;
{ This procedure displays the answer in  }
{ a nice numerical format. It switches   }
{ the title of the table displaying the  }
{ answer depending on the user's menu    }
{ choice.                                }
BEGIN
     WRITELN;
     WRITELN('        Degree');
     IF (UserChoice = 'a') THEN
        WRITELN('Celcius    | Fahrenheit');
     IF (UserChoice = 'b') THEN
        WRITELN('FahrenHeit |   Celcius');
     WRITELN('------------------------');
     WRITE(UserInput:8:2);
     WRITE('   |   ');
     WRITE(Answer:8:2);
     WRITELN;
     WRITELN;
END;

{****************************************}
BEGIN
   UserChoice := 'q';
   WHILE (UserChoice <> 'x') DO
      BEGIN
         ShowTheMenu;
         GetUserChoice;
         IF (UserChoice = 'a') OR
            (UserChoice = 'b') THEN
           BEGIN
              GetNumberToConvert;
              DoTheConversion;
              DisplayTheAnswer;
              Wait;
           END;
      END;
END.
