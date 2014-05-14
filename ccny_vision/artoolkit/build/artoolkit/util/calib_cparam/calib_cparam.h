#define  LINE_MAX     26
#define  LOOP_MAX     10

#define  L_HORIZONTAL  0
#define  L_VERTICAL    1

void   initLineModel( int   *line_num, int *loop_num,
                      int    line_mode[LINE_MAX],
                      double inter_coord[LOOP_MAX][LINE_MAX][LINE_MAX][3] );
