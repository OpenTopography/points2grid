#ifndef _GRID_POINT_H_
#define _GRID_POINT_H_

typedef struct _GridPoint
{
    double Zmin;
    double Zmax;
    double Zmean;
    unsigned int count;
    double Zidw;
    double sum;
    int empty;
    int filled;
} GridPoint;

#endif
