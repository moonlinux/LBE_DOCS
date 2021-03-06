typedef struct {
	float m[4][4];
} lbeMatrix;

typedef struct {
	float v[4];
} lbeVector;

void lbeProjection(lbeMatrix *result, float l, float r, float b, float t, float n, float f);
void lbeOrthoProjection(lbeMatrix *result, float l, float r, float b, float t, float n, float f);
void lbeTranspose(lbeMatrix *mat);
void lbeLoadIdentity (lbeMatrix *resultado);
void lbeRotate (lbeMatrix *resultado, float deg, float u, float v, float w );
void lbeSimpleRotate (lbeMatrix *resultado, float deg, int u, int v, int w );
void lbeMatrixMultiply (lbeMatrix *resultado, lbeMatrix *matrix_a, lbeMatrix *matrix_b);
void lbePrintMatrix(lbeMatrix *mat);
