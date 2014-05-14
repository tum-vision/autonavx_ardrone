#ifndef __object_h__
#define __object_h__


#define   OBJECT_MAX       30

#ifdef __cplusplus
extern "C" {
#endif	

typedef struct {
    char       name[256];
    int        id;
    int        visible;
    double     marker_coord[4][2];
    double     trans[3][4];
	int        vrml_id;
	int        vrml_id_orig;
    double     marker_width;
    double     marker_center[2];
} ObjectData_T;

ObjectData_T  *read_VRMLdata (char *name, int *objectnum);

#ifdef __cplusplus
}
#endif	

#endif // __object_h__
