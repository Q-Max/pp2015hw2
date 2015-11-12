#include<stdio.h>
#include<stdlib.h>
#include<X11/Xlib.h>
//#include <unistd.h>
#include<math.h>

#define G 6.67e-11

struct body{
double x;
double y;
double vx;
double vy;
};

void initGraph(int width,int height);
void draw(int x,int y);
inline void computeForce(struct body *bodies ,double m, double *x, double *y, int N);
inline void clear(int width,int height);

GC gc;
Display *display;
Window window;      //initialization for a window
int screen;         //which screen 

void initGraph(int width,int height)
{
	/* open connection with the server */ 
	display = XOpenDisplay(NULL);
	if(display == NULL) {
		fprintf(stderr, "cannot open display\n");
		exit(1);
	}

	screen = DefaultScreen(display);

	/* set window position */
	int x = 0;
	int y = 0;

	/* border width in pixels */
	int border_width = 0;

	/* create window */
	window = XCreateSimpleWindow(display, RootWindow(display, screen), x, y, width, height, border_width, BlackPixel(display, screen), WhitePixel(display, screen));
	
	/* create graph */
	XGCValues values;
	long valuemask = 0;
	
	gc = XCreateGC(display, window, valuemask, &values);
	//XSetBackground (display, gc, WhitePixel (display, screen));
	XSetForeground (display, gc, BlackPixel (display, screen));
	XSetBackground(display, gc, 0X0000FF00);
	XSetLineAttributes (display, gc, 1, LineSolid, CapRound, JoinRound);
	
	/* map(show) the window */
	XMapWindow(display, window);
	XSync(display, 0);

	/* draw rectangle */
	XSetForeground(display,gc,BlackPixel(display,screen));
	XFillRectangle(display,window,gc,0,0,width,height);
	XFlush(display);
}

inline void draw(int x,int y)
{
	/* draw point */
	XSetForeground(display,gc,WhitePixel(display,screen));
	XDrawPoint (display, window, gc, x, y);
}

const double PI = 3.14159;

int main(int argc,char *argv[])
{
	if(argc<8||argc>8&&argc!=12){
		puts("error in arguments");
		puts("\"./hw2_xxx #threads m T t FILE theta disable\" or");
		puts("\"./hw2_xxx #threads m T t FILE theta enable xmin ymin length Length\"");
		exit(0);

	}
	const int n = atoi(argv[1]);
	const double m = atof(argv[2]);
	const int T = atoi(argv[3]);
	const double t = atof(argv[4]);
	const char *filename = argv[5];
	const double theta = atof(argv[6]);
  const double unit;
	int enableX11;
	int xmin, ymin, length, x11Length;
	int acc_t;
	double *Fx, *Fy;
	const int N;
	struct body *bodies, *bodies_new;
	FILE *fp;
	if(!strcmp(argv[7],"enable"))
		enableX11 = 1;
	else if(!strcmp(argv[7],"disable"))
		enableX11 = 0;
	else{
		puts("error in X11 flag.\nEXIT");
		exit(0);
	}
	if(enableX11){
		xmin = atoi(argv[8]);
		ymin = atoi(argv[9]);
		length = atoi(argv[10]);
		x11Length = atoi(argv[11]);
	}
	int i, x, y;
	if(enableX11)initGraph(x11Length, x11Length);
	/*for( i = 0; i < 100; i++ )
		draw((int)((200*cos(i/100.0*2*PI))+250), 250+(int)(200*sin(i/100.0*2*PI)));
	for( i = 0; i < 50; i++)
		draw((int)((100*cos(i/50.0*2*PI))+250), 250+(int)(100*sin(i/50.0*2*PI)));*/
	
	fp = fopen(filename, "r");
	if(fp==NULL){
		puts("file error, EXIT");
		exit(0);
	}
	fscanf(fp,"%d",&N);
	printf("%d\n", N);
	bodies = (struct body*)malloc(sizeof(struct body)*N);
	bodies_new = (struct body*)malloc(sizeof(struct body)*N);
  Fx = (double*)malloc(sizeof(double)*N);
  Fy = (double*)malloc(sizeof(double)*N);
	for (i=0; i<N; i++){
		fscanf(fp,"%lf %lf %lf %lf",&bodies[i].x, &bodies[i].y, &bodies[i].vx, &bodies[i].vy);
		printf("%lf %lf %lf %lf\n", bodies[i].x, bodies[i].y, bodies[i].vx, bodies[i].vy);
	}
	for (acc_t=0; acc_t<T; acc_t++) {
		computeForce(bodies, m, Fx, Fy, N);
		for (i=0; i<N; i++) {
			//Compute_Force(i); // compute force in O(N^2)
			bodies_new[i].vx = bodies[i].vx + Fx[i] * t / m; // compute new velocity
			bodies_new[i].vy = bodies[i].vy + Fy[i] * t / m; // compute new velocity
			bodies_new[i].x = bodies[i].x + bodies_new[i].vx * t; // compute new position
			bodies_new[i].y = bodies[i].y + bodies_new[i].vy * t; // compute new position
		}
		for(i=0; i<N; i++){
      bodies[i] = bodies_new[i];
			/*bodies[i].x = x_new[i]; // update position
			bodies[i].y = y_new[i];
			bodies[i].vx = vx_new[i]; // update velocity
			bodies[i].vy = vy_new[i];*/
		}
	}
	if(enableX11){
    clear(length, length);
		for(i=0;i<N;i++) {
			x=(bodies[i].x-xmin)*unit;
			y=(bodies[i].y-ymin)*unit;
			if(x>0&&x<length&&y>0&&y<length)
				draw(x,y);
		}
    XFlush(display);
  }    
	sleep(3);
	return 0;
}
inline void computeForce(struct body *bodies ,double m, double *Fx, double *Fy, int N){
  int i, j;
  double fxt, fyt, r;
  for(i=0;i<N;i++){
    fxt=0;
    fyt=0;
    for(j=0;j<N;j++){
      if(i==j)
        continue;
      r = sqrt( (bodies[i].x-bodies[j].x)*(bodies[i].x-bodies[j].x) + (bodies[i].y-bodies[j].y)*(bodies[i].y-bodies[j].y) );
      if(r==0)
        puts("OAQQQQQQQQQQQQQQQQQQQQQQQQQ");
      else{
        fxt += ((G * m * m) * (bodies[j].x-bodies[i].x) / (r * r * r));
        fyt += ((G * m * m) * (bodies[j].y-bodies[i].y) / (r * r * r));
      }
    }
    Fx[i] = fxt;
    Fy[i] = fyt;
  }
}
inline void clear(int width,int height) {
	XSetForeground(display,gc,BlackPixel(display,screen));
	XFillRectangle(display,window,gc,0,0,width,height);
}