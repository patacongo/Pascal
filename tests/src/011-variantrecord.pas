program main(input, output);

type
  paytype = (salaried, hourly);

var
  employee1, employee2 :
    record
      id   : integer;
      dept : integer;
      age  : integer;
      case payclass: paytype of
        salaried:
          (monthlyrate : real;
           startdate : integer);
        hourly:
          (rateperhour : real;
           reghours : integer;
           overtime : integer);
    end;

begin
  employee1.id          := 001234;
  employee1.dept        := 12;
  employee1.age         := 38;
  employee1.payclass    := hourly;
  employee1.rateperhour := 2.75;
  employee1.reghours    := 40;
  employee1.overtime    := 3;
  writeln(employee1.rateperhour, employee1.reghours, employee1.overtime);

  {this should bomb as there is no monthlyrate because payclass=hourly}

  writeln(employee1.monthlyrate);

  employee2.payclass    := salaried;
  employee2.monthlyrate := 575.0;
  employee2.startdate   := 13085;

  {this should bomb as there are no rateperhour, etc. because payclass=salaried}

  writeln(employee2.rateperhour, employee2.reghours. employee2.overtime);
end.