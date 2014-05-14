#ifndef OBJECT_H
#define OBJECT_H


#define   OBJECT_MAX       10

typedef struct {
    char       name[256];
    int        id;
    int        visible;
    double     marker_width;
    double     trans[3][4];
} ObjectData_T;

ObjectData_T  *read_objectdata( char *name, int *objectnum );

#endif
