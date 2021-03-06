#include "mex.h"
#include <memory.h>
#include <math.h>
#include <time.h>
#include <iostream>
#include <algorithm>
#include <assert.h>

using namespace std;

// a' * b
double v_times(double* a, double* b, int d){
    double rtn = 0;
    for (int i = 0; i < d; ++i){
        rtn += a[i] * b[i];
    }
    return rtn;
}

double* v_times_v(double* a, double* b, int d){
    double* m = new double[d*d];
    for (int i = 0; i < d; ++i)
        for (int j = 0; j < d; ++j)
            m[i*d+j] = a[i]*b[j];
    return m;
}

// mat1 [m*n] x mat2 [n*p]= mat3 [m*p]
double* v_times_m(double* v, double* m, int d){
   double* v_new = new double[d];
   for (int i = 0; i < d; ++i){
        v_new[i] = v_times(v, m+i*d, d);
   }
   return v_new;
}


// v = v .* scale
void times_scale(double* v,  double scale, int d){
    for (int i = 0; i < d; ++i){
        v[i] = v[i] * scale;
    }
}

double* v_plus(double* a, double* b, int d){
    for (int i = 0; i < d; ++i){
        a[i] = a[i] + b[i];
    }
    return a;
}

double min(double a, double b){
	if(a < b)
	{b = a;}
	return b;
}

double max(double a, double b){
	if(a > b)
	{b = a;}
	return b;
}


#define NUMBER_OF_FIELDS (sizeof(field_names)/sizeof(*field_names))

void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[]) {
                     
    if (nrhs !=3){
        mexErrMsgTxt("Input agruments number != 3");
    } 
    
    /* =========================== INPUT =========================== */
    
    double* Y_ptr = mxGetPr(prhs[0]);
	int Y = Y_ptr[0];
    double* X_init = mxGetPr(prhs[1]);
	int D = mxGetM(prhs[1]);
	double* X = new double[D];
	memcpy(X, X_init, D * sizeof(double));
    /* -------- options ---------- */

    mxArray* w_ptr = mxGetField(prhs[2],0,"w");
    if (!w_ptr) mexErrMsgTxt("model.w is null");
	double* w_init = mxGetPr(w_ptr);
	double* w = new double[D];
	memcpy(w, w_init, D * sizeof(double));
    
    mxArray* t_ptr = mxGetField(prhs[2],0,"t");
    if (!t_ptr) mexErrMsgTxt("model.t is null");
	double t = mxGetScalar(t_ptr);

    mxArray* loss_type_ptr = mxGetField(prhs[2],0,"loss_type");
    if (!loss_type_ptr) mexErrMsgTxt("model.loss_type is null");
	double loss_type = mxGetScalar(loss_type_ptr);

    mxArray* C_ptr = mxGetField(prhs[2],0,"C");
    if (!C_ptr) mexErrMsgTxt("model.C is null");
	double C = mxGetScalar(C_ptr);
    
    /* =========================== INIT OUTPUT =========================== */

	int hat_y_t;
	double l_t;

    /* =========================== PRODUCE =========================== */
    
    double f_t;
	f_t = v_times(w, X, D);	

	if (f_t >= 0)
       hat_y_t = 1;
    else
       hat_y_t = -1;

	double eta_t   = C/sqrt(t);	

	switch (int(loss_type)){
	case 0:            // 0-1 loss
		if (hat_y_t == Y)
		   l_t = 0;
		else 
		   l_t = 1;
		if (l_t > 0){			
			times_scale(X, Y*eta_t, D);
			w = v_plus(w, X, D);
		}
    break;
	case 1:            // hinge loss
		l_t = max(0,1-f_t*Y);
		if (l_t > 0){
			times_scale(X, Y*eta_t, D);
			w = v_plus(w, X, D);
		}
    break;
	case 2:            // logistic loss		
		l_t = log(1+exp(-Y*f_t));
		if (l_t > 0){			
			times_scale(X,Y*eta_t/(1+exp(Y*f_t)),D);
			w = v_plus(w, X, D);			
		}
    break;
	case 3:            // square loss		
		l_t = 0.5*(Y-f_t)*(Y-f_t);
		if (l_t > 0){
			times_scale(X, eta_t*(Y-f_t), D);
			w = v_plus(w, X, D);			
		}
    break;
	default:            // default
		mexErrMsgTxt("invalid loss_type!");
    break;
	}  

	t = t+1;
    
    /* =========================== UPDATE OUTPUT =========================== */

	const char *field_names[] = {"w","t","loss_type","C"};
    plhs[0] = mxCreateStructMatrix(1, 1, NUMBER_OF_FIELDS, field_names);

	mxArray* w_mxarray = mxCreateDoubleMatrix(D,1,mxREAL);
	double* w_ptr_mxarray = mxGetPr(w_mxarray);
	memcpy(w_ptr_mxarray, w, D * sizeof(double));
	mxSetField(plhs[0], 0, "w", w_mxarray);
    
    mxArray* t_mxarray = mxCreateDoubleMatrix(1,1,mxREAL);
    double* t_ptr_mxarray = mxGetPr(t_mxarray);
    t_ptr_mxarray[0] = t;
    mxSetField(plhs[0], 0, "t", t_mxarray); 

    mxArray* loss_type_mxarray = mxCreateDoubleMatrix(1,1,mxREAL);
    double* loss_type_ptr_mxarray = mxGetPr(loss_type_mxarray);
    loss_type_ptr_mxarray[0] = loss_type;
    mxSetField(plhs[0], 0, "loss_type", loss_type_mxarray);    

	mxArray* C_mxarray = mxCreateDoubleMatrix(1,1,mxREAL);
    double* C_ptr_mxarray = mxGetPr(C_mxarray);
    C_ptr_mxarray[0] = C;
    mxSetField(plhs[0], 0, "C", C_mxarray); 
   
    plhs[1] = mxCreateDoubleMatrix(1,1,mxREAL);
	double* hat_y_t_ptr = mxGetPr(plhs[1]); 
	hat_y_t_ptr[0] =  hat_y_t;
	plhs[2] = mxCreateDoubleMatrix(1,1,mxREAL);
	double* l_t_ptr = mxGetPr(plhs[2]); 
    l_t_ptr[0] = l_t;
    
    /* =========================== FREE MEMORY =========================== */

	delete[] w;
	delete[] X;    
    
}
