/*
 * Funciones de transformación (rotación, traslación, escalado) para el proyecto LBE
 *
 */
#include "lbeTransform.h"
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <GLES2/gl2.h>

#define PI 3.1415926535897932384626433832795f

void lbeLoadIdentity (lbeMatrix *resultado){
	lbeMatrix res = {{{1, 0, 0, 0},
		          {0, 1, 0, 0},
		          {0, 0, 1, 0},
		          {0, 0, 0, 1}}};
	
	memcpy ((*resultado).m, res.m, sizeof ((*resultado).m));
}

void lbeMatrixMultiply (lbeMatrix *resultado, lbeMatrix *matrix_a, lbeMatrix *matrix_b){
	int i,j;
	float accum = 0.0f;
	float res[4][4] = {{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}};
	//memcpy (resultado, &res, sizeof(resultado));
	//La idea es que dejamos fija una fila de la primera matriz y vamos avanzando en la segunda por columnas.
	//En bloques porque en uno solo habría que introducir un iterador más y eso complica la lectura.
	for (i = 0; i < 4; i++){
		for (j = 0; j < 4; j++){
				accum = accum + (*matrix_a).m[0][j] * (*matrix_b).m[j][i];
		}
		res[0][i] = accum; 
		accum = 0.0f;
	}
	
	for (i = 0; i < 4; i++){
		for (j = 0; j < 4; j++){
				accum = accum + (*matrix_a).m[1][j] * (*matrix_b).m[j][i];
		}
		res[1][i] = accum; 
		accum = 0.0f;
	}
	
	for (i = 0; i < 4; i++){
		for (j = 0; j < 4; j++){
				accum = accum + (*matrix_a).m[2][j] * (*matrix_b).m[j][i];
		}
		res[2][i] = accum; 
		accum = 0.0f;
	}

	for (i = 0; i < 4; i++){
		for (j = 0; j < 4; j++){
				accum = accum + (*matrix_a).m[3][j] * (*matrix_b).m[j][i];
		}
		res[3][i] = accum; 
		accum = 0.0f;
	}
	memcpy((*resultado).m, res, sizeof((*resultado).m));
}

void lbeMatrixVectorMultiply (lbeVector *resultado, lbeMatrix *mat, lbeVector *vec){
	int i,j;	
	float accum = 0.0f;
	lbeVector res;
	for (i = 0; i < 4; i++){
		for (j = 0; j < 4; j++){
			accum = accum + (*mat).m[i][j] * (*vec).v[j];	
		}
		res.v[i] = accum;
		accum = 0.0f;
	}
	memcpy ((*resultado).v, res.v, sizeof((*resultado).v));
}

/* La función de rotación asume la interpretación de OpenGL "clásico": las transformaciones se hacen 
 * de manera "acumulativa sobre sistema local", o sea, según sistema de coordenadas local al objeto o al 
 * sistema de referencia. Esto quiere decir que las sucesivas transformaciones entran postmultiplicando en 
 * el matrix stack.
 * A este nivel, se considera la rotación como RV+, por lo que será a un nivel de abstracción más alto donde 
 * se implemente la inversión de la transformación si se hace sobre sistema de referencia.  
 * Recuerda: por una parte está el cómo se acumulan las transformaciones, y por otra si consideramos
 * que se transforma el modelo o el sistema de referencia.
 */

/*Los radianes son una forma ingeniosa de especificar un ángulo: como se trata del arco cuya longitud es igual al
* radio, las circunferencias más pequeñas tienen un tamaño de radián más pequeño y tienen el mismo número de 
* radianes que las grandes. Son un buen sustituto a los grados, que son sólo una división en partes iguales.	
*/

/*Te darás cuenta de que este layout no se corresponde con la colocación que esperarías para operar directamente
* con esta matriz (en postmul o premul): es que eso es a parte, lo hace OpenGL internamente en base a cómo
* traen los shaders definido el producto de matriz con vector. NO ESPERES una relación inmediata entre el layout
* que espera OpenGL y cómo han de estar los elementos para operar con la matriz: voy a colocar el vector de 
* traslación en lo que parece ser la última fila (en realidad no hay ninguna fila porque OpenGL espera la matriz
* en una formación unidimensional), aunque te extrañe porque debería estar en la última columna si pones los 
* vectores unitarios por filas, pero por eso, porque es un layout para OpenGL y no se corresponde con el layout
* para operar con la matriz directamente: no busques tal relación porque sólo encontrarás confusión.
*/

/*De momento, aceptamos que es posible recibir más de una rotación en torno a eje (por ejemplo con 1,0,1) pero
 * como es distinto rotar en torno a X y luego en torno a Z que al revés (lo mires como lo mires, debido a
 * que el producto de matrices no es conmutativo), fijamos que el órden en que que harán rotaciones sucesivas 
 * en torno a distintos ejes siempre será: primero en torno a X, después respecto a Y y por último respecto a Z.
 * Queda pendiente implementar un órden concreto, usando valores distintos de 1 y de 0.
*/

/*Recuerda que se acumulan las sucesivas transformaciones emulando el mismo modo que usa OpenGL clásico:
 * acumulándolas sobre el sistema local al modelo o el sistema de referencia del observador, o sea, que cada
 * nueva matriz entra postmultiplicando en el matrix stack.*/  

/*Por si te lias con lo de "matriz_identidad * rot_x * vértice", recuerda que, aunque efectivamente puedes
 * multiplicar primero las matrices entre sí respetando el órden, eso no cambia nada respecto a premultiplicar
 * el vector por rot_x y luego premultiplicar de nuevo el resultado por matriz_identidad, ya que si eligieses
 * multiplicar primero las matrices entre sí, un producto matricial es cada fila por todas las columnas, por
 * lo que al final, si premultiplicas o postmultiplicas por una identidad, la matriz de rotación queda intacta,
 * como es de esperar
 */

void lbeRotate (lbeMatrix *resultado, int axis_x, int axis_y, int axis_z, float deg){
	
	int i;
	float rad = 2 * PI / 360 * deg ;

	float c = cos (rad);
	float s = sin (rad);

	lbeMatrix mat_rx = {{{1, 0, 0, 0}, 
	       	      	     {0, c,-s, 0}, 
	       	             {0, s, c, 0}, 
	       	             {0, 0, 0, 1}}};

	lbeMatrix mat_ry = {{{c, 0, s, 0}, 
	       	             {0, 1, 0, 0}, 
	       	             {-s,0, c, 0}, 
	       	             {0, 0, 0, 1}}};

	lbeMatrix mat_rz = {{{c,-s, 0, 0}, 
	       	      	     {s, c, 0, 0}, 
	       	             {0, 0, 1, 0}, 
	       	             {0, 0, 0, 1}}};

	//No debemos pasar result como matriz resultado en el producto porque ya se pasa como factor, y 
	//es alterada durante la multiplicación al ser el resultado.Por eso recogemos sobre res y luego hacemos memcp 
	if (axis_x == 1) {	
		//printf ("mult X\n");
		lbeMatrixMultiply(resultado, resultado, &mat_rx);		
	}
	if (axis_y == 1) {	
		//printf ("mult Y\n");
		lbeMatrixMultiply(resultado, resultado, &mat_ry);		
	}
	if (axis_z == 1) {	
		//printf ("mult Z\n");
		lbeMatrixMultiply(resultado, resultado, &mat_rz);		
	}
}

/*Caso más general de proyección con perspectiva: frustrum de plano cercano no centrado en 0.0.0
* No se realiza cambio de RHS a LHS, así que las coordenadas NDC resultantes (en GLES realmente también se
* esperan coordenadas NDC, de ahí que si no llevas a cabo ninguna proyección, dibujes lo que dibujes sólo
* se va a ver lo que quede dentro del cubo X [1, -1], Y [1, -1], Z [1, -1]) van a estar respecto a un sistema
* tal que Z positivo es hacia el observador, Z negativo es hacia adelante, X positivo es hacia la derecha e
* Y positivo es hacia arriba. 
* Como no se hace este cambio de RHS a LHS, la fila de Z pierde los signos negativos respecto al desarrollo
* de la sección 6. 
*/
void lbeProjection(lbeMatrix *result, float l, float r, float b, float t, float n, float f) {
	float t11 = (2 * n) / (r - l);
	float t13 = (r + l) / (r - l);
	
	float t22 = (2 * n) / (t - b);
	float t23 = (t + b) / (t - b);

	float t33 =   (f + n) / (f - n);
	float t34 =   (2 * f) / (f - n); 
	
	lbeMatrix res = {{{t11,   0, t13,   0},
			  {  0, t22, t23,   0}, 
			  {  0,   0, t33, t34},
			  {  0,   0,  1,   0}				
	}};	
	lbePrintMatrix(&res);	
	
	memcpy ((*result).m, res.m, sizeof((*result).m));
}

void lbePrintMatrix(lbeMatrix *mat) {
	int i,j;
	printf ("Matriz resultante: \n");
	for (i=0; i<4; i++){
		for (j=0; j<4; j++){
			printf ("%f ", (*mat).m[i][j]);		
		}
		printf ("\n");
	}
}

void lbeCheckGLError (){
	//Función de checkeo de flags de error activos, para ser usada tras llamadas dudosas.
	GLenum err  = glGetError();
 
        while(err!=GL_NO_ERROR) {
                char* error;
                switch(err) {
                        case GL_INVALID_OPERATION:      error="INVALID_OPERATION";      break;
                        case GL_INVALID_ENUM:           error="INVALID_ENUM";           break;
                        case GL_INVALID_VALUE:          error="INVALID_VALUE";          break;
                        case GL_OUT_OF_MEMORY:          error="OUT_OF_MEMORY";          break;
                        case GL_INVALID_FRAMEBUFFER_OPERATION: error="INVALID_FRAMEBUFFER_OPERATION";  break;
                	default: error = "OTHER ERROR"; break;
		}

		printf ("ERROR - error numerado de OpenGL: %s\n", error); 
                err=glGetError();
        }
}
