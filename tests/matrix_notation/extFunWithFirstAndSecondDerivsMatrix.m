function [y dy d2y]=extFunWithFirstAndSecondDerivsMatrix(a,b,c)
y=[a(1), 5*b(1); 3*a(2), 7*b(2)]*c.^2;

dy = [ c(1)^2 0        5*c(2)^2 0        2*a(1)*c(1) 10*b(1)*c(2);
       0      3*c(1)^2 0        7*c(2)^2 6*a(2)*c(1) 14*b(2)*c(2) ];

d2y=zeros(2,6,6);
d2y(1,1,5) = 2*c(1);
d2y(1,3,6) = 10*c(2);
d2y(1,5,1) = 2*c(1);
d2y(1,5,5) = 2*a(1);
d2y(1,6,3) = 10*c(2);
d2y(1,6,6) = 10*b(1);
d2y(2,2,5) = 6*c(1);
d2y(2,4,6) = 14*c(2);
d2y(2,5,2) = 6*c(1);
d2y(2,5,5) = 6*a(2);
d2y(2,6,4) = 14*c(2);
d2y(2,6,6) = 14*b(2);
end
