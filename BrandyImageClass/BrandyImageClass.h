#ifndef _class_
#define _class_
#include <cmath>
using namespace std;


/*
What even is this? It's my algorithm to help the IRIS V navigate! What should happen by the time this is done is that you give it an image in matrix form, and it gives you a linked list of lines that make up a block I in the image. It works becase the block I that it is looking for has a bunch of LEDs that make it up. So it extracts the bright points from the image, and stores them. Those are all we really care about anyway, so it acts kind of like a sparse matrix of interesting points. So the matrix image is converted to something called a BrandyImage, which has three parts. 1. The inherent qualities of the image, average brightness and size are the two big ones. These are just so that the remaining parts make sense in the context of navigation. 2. BrightPoints. A brandyimage contains a linked list of bright points, a sparse matrix of points of interest, more or less. 3. AndresLines. Named after Andres, because he came up with a brilliant way to store the lines in a way that we can work with them without storing much memory. The AndresLines are what are eventually being exported. The other things just exist to give us our final product: a linked list of 10 lines. How can we be sure they are the right lines? Well, that's where the line creation and scoring algorithm comes in. The scoring is super complicated and I haven't figured out how to do it yet, but it is something that I'll get around to. Anyway, here's the creation piece

1. Extract bright points from iamge
2. Order points in a linked list based on the closest distance to the BWAC, the brightness weighted average coordinate. It is the "center of brightness" (a la center of mass) for all the bright points in the image. It only accounts for the brightness from brightpoints, not for the whole of the image. Dull points contribute nothing here. 
3. Start by creating a line from the first BrightPoint in the list to the second one. Score it. Then make a line from the first BrightPoint to the third. score it. Keep going. After scoring each line, add it to the linked list of AndresLines, only storing the top 10 and ignoring the rest. Repeat 3-15 times for each point. Note: to avoid redundancy, if you are on point #3, make a line from point 3 to 4 as your first try, not 3 to 1, because that is the same as 1 to 3. Anyway...
4. Keep doing this until a predetermined threshold for total score for all the lines is reached

*/


class BrightPointClass
{
/*
DESCRIPTION
Each instance of the following class is designed to be a data structure called a bright point. Bright points are bits of data that are extracted from an image, and will be used to help recognize the pattern on the banner. Storing the data as bright points as opposed to storing the image data for every pixel is a much more effiecient way to store the data. Bright points are stored in a linked list, whose head is pointed to in the BrandyImage. BrightPoints are made from squares. Within the matrix image, a once a certain cluster of pixels passes some kind of threshold, it is recognized as a point on the image worth storing as a BrightPoint. 

*/

public:
//CONSTRUCTORS
    BrightPointClass();
//DESTRUCTOR
    ~BrightPointClass();
//FUNCTIONS


//VARIABLES
  //PUBLIC VARIABLES
BrightPointClass * next;
unsigned int row;//The row coord of the center pixel of the BrightPoint
unsigned int col;//The col coord of the center pixel of the BrightPoint
unsigned int Brightness// the total brightness value

private:
  //PRIVATE VARIABLES
};



class AndresLineClass
{
/*
This is a reprsentation of a line using a single point, A and a unit vector pointing in the direction the line is going, (unit vector B). That way calculating the distance between the line and the point, c can just be |b x (c-a)| where b is our unit vector, and c-a is the vector pointing from point A to point C, and that x is the cross product, and || is the magnitude. 
*/

public:
//CONSTRUCTORS
    AndresLineClass();//The constructor for the head
    AndresLineClass(AndresLineClass * elt);//elt is a pointer to an element in the list, not necessarily the last one
//DESTRUCTOR
    ~AndresLineClass();
//FUNCTIONS
    double DistanceFromLine(int row, int col)//Returns the distance between the point passed to the function as row,col and the line

//VARIABLES
  //PUBLIC VARIABLES
AndresLineClass * next;//The next elt in the list
int LineNumber;//numbered 0-9 since the list should contain no more than 10 elements. If the position is too large, than you 
int ARow;//
int ACol;//
double Bangle;//The angle in radians of the unit vector b
private:
  //PRIVATE VARIABLES
};





class BrandyImageClass
{
/*
DESCRIPTION
Each instance of the following class is designed to be a data structure that will be a compact way of storing image data for the IRIS 5's location recognition. A seperate program will take the standard matrix way of storing images and convert it into this much more compact form, a BrandyImage. I named the data structure / class a "BrandyImage" because Brandy is my nickname and I like the idea of naming my creations after myself. Also I'm fairly sure no one is going to confuse a "BrandyImage" with any other kind of data structure/ format. 

BWAC= Brightness weighted average center. The point calculated to be the "center of brightness" of the image

*/

public:
//CONSTRUCTORS
    BrandyImageClass(int rows, int cols, int *RedPointer, int *BluePointer int *GreenPointer);
/*
*/
//DESTRUCTOR
    ~BrandyImageClass();
//FUNCTIONS
    void addBP(int r, int c, int B);//Adds a BP to the end of the BP list
    void findBWA();//Finds the BWA row and column and changes the BWARow and BWACol variables
    void sortBPs();//Sorts the linked list of BrightPoints by distances from BWAC. 
    void CreateLineInOrder(int Arow, int Acol, int Brow, int Bcol);//Adds a line to the linked list of lines
    int scoreline(AndresLineClass *MyLine);//Scores a single given line. The crux of our function
    int scoretotal();//Calculates the total score for the entire linked list of lines Once it reaches a threshold we are done

//VARIABLES
  //PUBLIC VARIABLES
BrightPointClass * BrightPointListHead;//Points to the head of the linked list of bright points
AndresLineClass * AndresLineListHead;//Points to the head of the linked list of scored lines. Should be no more than 10 elements long
int BWACRow;//Row coordinate of the BWAC
int BWACCol;
int BPListSize;//Number of bright points
unsigned int RunningAvgBrightness;//the average brightness of the whole image being counted as the image itself is being scanned, rather than after the fact
unsigned int RunningArea;//The total image area, being calculated as the image is scanned. Measured in pixels 
int ImageSizeRow;//The size of the parent image, in rows and columns
int ImageSizeCol;
bool islinelistfull; //we don't start scoring until the line list has 10 entries. Doesn't save much time, but it saves some. 
private:
  //PRIVATE VARIABLES
   
};


