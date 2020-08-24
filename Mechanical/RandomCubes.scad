SizeX = 100;
SizeY =100;
HP = 3;

difference()
{
    HP6();
    union()
    {
    translate([3,7.5,2])
      ScrewHole();
    translate([128.5-3,7.5,2])
      ScrewHole();
    translate([3,7.5+5.08*HP,2])
      ScrewHole();
    translate([128.5-3,7.5+5.08*HP,2])
      ScrewHole();
        }
    }

module HP6()
{  
    intersection()
    {
        rotate([0,0,-11])
          scale([6,6,1])
          RandomCubes();
        
        scale([128.5 ,30.0,10])
          cube(size=1, center=false);
    }
}
module RandomCubes()
{
    
for (x = [-1: SizeX]){
    for ( y = [-1:SizeY]){
        if((x+y)%2 == 0)
        {
        randScale = 0.01;    
        randoms = x*rands(-randScale,randScale,10);
        CubeSize=2*randoms[1]+1;
        translate([x, y, 2+(randoms[0]+randScale)*10])
        //translate([x, y, 0])        
        scale([CubeSize,CubeSize,3])
        //rotate([randoms[2]*20,randoms[3]*20,randoms[4]*20])
        rotate([0,0,randoms[4]*60])
        cube(size = 1, center = true);
        }
    }
}
scale([2*SizeX,2*SizeY,4])
  translate([0,0,0])
    cube(size =1, center = true);
}

module ScrewHole()
{   
    translate([0,0,-20])
        cylinder(h=20,r=3.2/2);
    cylinder (h=20, r=5/2);
}