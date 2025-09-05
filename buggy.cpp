#include <iostream>

struct Point {
    int x, y;

    Point () : x(), y() {}
    Point (int _x, int _y) : x(_x), y(_y) {}
};

class Shape {
public:
    int vertices;
    Point** points;

    Shape (int _vertices) {
        vertices = _vertices;
        points = new Point*[vertices+1];
        for (int i = 0; i <= vertices; i++) {
            points[i] = new Point();
        }
    }

    ~Shape () {
        for (int i = 0; i <= vertices; i++) {
            delete points[i];
        }
        delete[] points;
    }

    void addPoints (Point pts[]) {
        for (int i = 0; i <= vertices; i++) {
            *points[i] = pts[i % vertices];
        }
    }

    double area () {
        int temp = 0;
        for (int i = 0; i < vertices; i++) {
            int lhs = points[i]->x * points[(i+1)%vertices]->y;
            int rhs = points[(i+1)%vertices]->x * points[i]->y;
            temp += (lhs - rhs);
        }
        double area = abs(temp)/2.0;
        return area;
    }
};

int main () {
    // Create triangle points using three different methods
    Point tri1; // default constructor
    tri1.x = 0; tri1.y = 0;
    Point tri2 = {1, 2}; // aggregate initialization
    Point tri3(2, 0); // parameterized constructor

    Point triPts[3] = {tri1, tri2, tri3};
    Shape* tri = new Shape(3);
    tri->addPoints(triPts);

    // Create quadrilateral points
    Point quad1(0, 0);
    Point quad2(0, 2);
    Point quad3(2, 2);
    Point quad4(2, 0);

    Point quadPts[4] = {quad1, quad2, quad3, quad4};
    Shape* quad = new Shape(4);
    quad->addPoints(quadPts);

    // Print out area of tri and area of quad
    std::cout << "Triangle area: " << tri->area() << std::endl;
    std::cout << "Quadrilateral area: " << quad->area() << std::endl;

    delete tri;
    delete quad;
}
