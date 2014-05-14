#ifndef COMMAND_SUB_H
#define COMMAND_SUB_H

#include "paddle.h"

/* on item structure */
typedef struct {
  double pos[2];
  int onpaddle;
} Item;

/* list of items */
typedef struct {
  int itemnum;
  Item item[256];
} ItemList;

/* item which is on the paddle */
typedef struct {
    int     item;
    double  angle;
    double  x, y;
} PaddleItemInfo;

/* shaking gesture to remove a pattern */
int check_shake   ( double paddleTrans[3][4], int f );

/* inclining gesture to put an object on the ground*/
int check_incline ( double paddleTrans[3][4], double baseTrans[3][4], double *angle );

/* picking gesture to take an object from the ground*/
int check_pickup(double card_trans[3][4], double base_trans[3][4], ItemList* itlist, double* angle);

#endif
