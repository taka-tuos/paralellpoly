#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <stdint.h>
#include <omp.h>

uint32_t *out;

#define ABSI(n) ((n) > 0 ? (n) : -(n))

void arrayline(int *ay, int x1, int y1, int x2, int y2)
{
	int steep = ABSI(y2-y1) > ABSI(x2-x1);
	
	if(steep) {
		int tmp;
		tmp=x1;
		x1=y1;
		y1=tmp;
		
		tmp=x2;
		x2=y2;
		y2=tmp;
	}
	
	if(x1>x2) {
		int tmp;
		tmp=x1;
		x1=x2;
		x2=tmp;
		
		tmp=y1;
		y1=y2;
		y2=tmp;
	}
	
	int dx=ABSI(x2-x1);
	int dy=ABSI(y2-y1);
	
	int err=dx/2;
	int sy;
	
	sy = y1 < y2 ? 1 : -1;
	
	int y = y1;
	
	for(int x=x1;x<=x2;x++) {
		if(steep) {
			ay[x] = y;
			//out[x*256+y]=0xffff0000;
		} else {
			ay[y] = x;
			//out[y*256+x]=0xff00ff00;
		}
		
		err -= dy;
		if(err < 0) {
			y+=sy;
			err+=dx;
		}
	}
}

#define INTERP(xi,xi1,yi,yi1,x) (yi + ((( yi1 - yi ) * ( x - xi )) / ( xi1 - xi )))

void pppoly(int *xv, int *yv, int *uv, int *vv)
{
	int s[3][1024];
	int m[3][4]; // min x,max x,min y,max y
	
	int miny=1024,maxy=0;
	
	for(int i=0;i<3;i++) memset(s,255,1024*sizeof(int));
	
	int x[3],y[3];
	int u[3],v[3];
	int mii,mai;
	
	for(int i=0;i<3;i++) {
		if(yv[i] > maxy) {
			maxy = yv[i];
			mai = i;
		}
		if(yv[i] < miny) {
			miny = yv[i];
			mii = i;
		}
	}
	
	int dmy1,dmy2,dmy3;
	unsigned int *tex = (unsigned int *)stbi_load("tex.png",&dmy1,&dmy2,&dmy3,4);
	
	for(int i=0;i<3;i++) {
		if(i == mii) {
			x[0] = xv[i];
			y[0] = yv[i];
			u[0] = uv[i];
			v[0] = vv[i];
			
		} else if(i == mai) {
			x[2] = xv[i];
			y[2] = yv[i];
			u[2] = uv[i];
			v[2] = vv[i];
		} else {
			x[1] = xv[i];
			y[1] = yv[i];
			u[1] = uv[i];
			v[1] = vv[i];
		}
	}
	
	for(int i=0;i<3;i++) {
		printf("v[%d]=%d,%d/%d,%d\n",i,xv[i],yv[i],x[i],y[i]);
	}
	
	for(int i=0;i<3;i++) {
		int x1=x[i],y1=y[i],x2=x[(i+1)%3],y2=y[(i+1)%3];
		m[i][0] = x1 < x2 ? x1 : x2;
		m[i][1] = x1 > x2 ? x1 : x2;
		m[i][2] = y1 < y2 ? y1 : y2;
		m[i][3] = y1 > y2 ? y1 : y2;
		
		printf("m[%d]=%d,%d,%d,%d\n",i,m[i][0],m[i][1],m[i][2],m[i][3]);
	}
	
	for(int i=0;i<3;i++) {
		arrayline(s[i],x[i],y[i],x[(i+1)%3],y[(i+1)%3]);
		printf("x[%d]=%d,%d\n",i,s[i][m[i][2]],s[i][m[i][3]]);
	}
	
	printf("min=%d,max=%d\n",miny,maxy);
	
#pragma omp parallel for
	for(int i=miny;i<maxy;i++) {
		int sa,sb,tmp;
		
		int ua,ub;
		int va,vb;
		
		if(m[0][2] <= i && m[0][3] > i) {
			sa = s[0][i];
			ua=INTERP(y[0],y[1],u[0],u[1],i);
			va=INTERP(y[0],y[1],v[0],v[1],i);
		}
		if(m[1][2] <= i && m[1][3] > i) {
			sa = s[1][i];
			ua=INTERP(y[1],y[2],u[1],u[2],i);
			va=INTERP(y[1],y[2],v[1],v[2],i);
		}
		if(m[2][2] <= i && m[2][3] > i) {
			sb = s[2][i];
			ub=INTERP(y[2],y[0],u[2],u[0],i);
			vb=INTERP(y[2],y[0],v[2],v[0],i);
		}
		
		if(sa > sb) {
			tmp=sa;
			sa=sb;
			sb=tmp;
			
			tmp=ua;
			ua=ub;
			ub=tmp;
			
			tmp=va;
			va=vb;
			vb=tmp;
		}
		
		for(int j=sa;j<sb;j++) {
			int g=INTERP(sa,sb,ua,ub,j);
			int b=INTERP(sa,sb,va,vb,j);
			//out[i*256+j]=0xff000000 | (g << 8) | (b << 16);
			out[i*256+j] = tex[b*256+g];
		}
	}
}

int main(void)
{
	int w=256,h=256;
	
	out = (uint32_t *)malloc(w*h*sizeof(uint32_t));
	
	memset(out,0x00,w*h*sizeof(uint32_t));
	
	int x[] = {160,30,240};
	int y[] = {30,120,240};
	int u[] = {0,0,255};
	int v[] = {0,255,255};
	
	pppoly(x,y,u,v);
	
	stbi_write_png("out.png",w,h,4,out,0);
	
	return 0;
}
