// simplix readme.txt 03. Dez. 2008
//
// Hints

Skilling:

The skilling is used to define the range of lap times for the gamers opponents.
This can be done on two separate levels.
The first level is the global level defined by Andrew Sumner and used for the Career mode of Mart Kelder.
If the supercarpackage of Andrew is installed, there is the file ...\torcs\config\raceman\extra\skill.xml.
It contains one parameter to be set in the range of 0.0 (fast) to 10.0 (slow).

To Enable/Disable the skilling for all cars of a robot there is a file
...\drivers\robotname\default.xml.
Here you can enable skilling (1) or disable it (0).

If needed, the individual skill of a car can be set in the
...\drivers\robotname\cartype\default.xml.
The parameter skill in "simplix private" can be set to 
  -1 (disable skilling for this car), 
 0.0 (normal skilling as defined in extra\skill.xml) or 
 values up to 10.0 (make car drive slower).

The individual skilling isn't needed here, the trb1 carset is balanced!


