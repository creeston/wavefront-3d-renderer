#include "helper.h"

void swapS(struct v2D *A, struct v2D *B) {
    int xaux, yaux;
    xaux = A->x;
    A->x = B->x;
    B->x = xaux;

    yaux = A->y;
    A->y = B->y;
    B->y = yaux;

    double aux;
    aux = A->depth;
    A->depth = B->depth;
    B->depth = aux;
}

void swap(double *x, double *y) {
    double aux;
    aux = *x;
    *x = *y;
    *y = aux;
}

void swapI(int *x, int *y) {
    int aux;
    aux = *x;
    *x = *y;
    *y = aux;
}
