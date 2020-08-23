SizeX = 20;
SizeY =8;

scale([1,1,0.5])
intersection()
{
    RandomCubes();
    scale([SizeX,SizeY,10])
      cube(size=1, center=false);
}
module RandomCubes()
{
    
for (x = [0: SizeX]){
    for ( y = [0:SizeY]){
        randoms = x*rands(-0.01,0.02,10);
        CubeSize=2*randoms[1]+1;
        //translate([x, y, randoms[0]])
        translate([x, y, (x+y)%2*0.3])        
        scale([CubeSize,CubeSize,1])
        //rotate([randoms[2]*20,randoms[3]*20,randoms[4]*20])
        rotate([0,0,randoms[4]*30])
        cube(size = 1, center = false);
    }
}
scale([SizeX,SizeY,2])
  translate([0,0,-0.8])
    cube(size =1, center = false);
}
