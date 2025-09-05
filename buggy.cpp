#include <iostream>

/* Nikita Kelwada
    CSCE 313 - Section 512
    Lab 1 
*/

struct Point {
    int x, y;

    Point () : x(), y() {}
    Point (int _x, int _y) : x(_x), y(_y) {}
};

class Shape {
    int vertices;
    Point** points;

// NOTE : Changed to public so the main() func and other code can cuse and delete any Shape objects 
public: 
    Shape (int _vertices) {
        vertices = _vertices;
        points = new Point*[vertices]; // NOTE: Took out the +1 to just vertices because it only needs 'vertices' number of points
        for (int i = 0; i < vertices; ++i){
            points[i] = nullptr;
        } 
    }

    // NOTE : Destructor would also be public so the objects can be deleted from outside the class if needed. It'll loop through and deletes dynamically created point objects then deletes the array of pointers so hopefully will help with preventing memory leaks
    ~Shape () {
        for (int i = 0; i < vertices; ++i){
            delete points[i];
        }
        delete[] points;
    }

    void addPoints (Point* pts) {
        for (int i = 0; i < vertices; i++) { 
            points[i] = new Point(pts[i].x, pts[i].y); // NOTE : The original code was copying the address of the points in pts to points, and also it was a raw mem copy which isn't the best option. Changed to creating new Point objects and copying the values of x and y
        }
    }

    double area () {
        int temp = 0;
        for (int i = 0; i < vertices; i++) {
            int j = (i + 1) % vertices;
            int lhs = points[i]->x * points[j]->y; // NOTE : Changed to ->  becuase points[i] is a pointer to a Point struct
            int rhs = (*points[j]).x * (*points[i]).y; // NOTE : Need to dereference the pointer first, then access x and y so used * and .
            temp += (lhs - rhs);
        }
        return abs(temp)/2.0; //NOTE : changed from &area and what was here before because &area was giving the address of the area function, and the formula for area was incorrect. 

    }
};

int main () {
    // FIXME: create the following points using the three different methods
    //        of defining structs:
    //          tri1 = (0, 0)
    //          tri2 = (1, 2)
    //          tri3 = (2, 0)

    Point tri1; tri1.x = 0; tri1.y = 0;
    Point tri2(1, 2); 
    Point tri3(2, 0); 

    // adding points to tri
    Point triPts[3] = {tri1, tri2, tri3};
    Shape* tri = new Shape(3);
    tri->addPoints(triPts);


    // FIXME: create the following points using your preferred struct
    //        definition:
    //          quad1 = (0, 0)
    //          quad2 = (0, 2)
    //          quad3 = (2, 2)
    //          quad4 = (2, 0)

    Point quad1(0, 0); 
    Point quad2(0, 2); 
    Point quad3{2, 2}; 
    Point quad4{2, 0}; 

    // adding points to quad
    Point quadPts[4] = {quad1, quad2, quad3, quad4};
    Shape* quad = new Shape(4);
    quad->addPoints(quadPts);

    std::cout << "Area of a triangle : " << tri->area() << std::endl;
    std::cout << "Area of a quadrilateral : " << quad->area() << std::endl;

    delete tri;
    delete quad;
    return 0;
}
